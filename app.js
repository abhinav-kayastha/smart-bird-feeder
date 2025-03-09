const express = require("express");
const mqtt = require("mqtt");
const path = require("path");
const bodyParser = require("body-parser");
const db = require("./config/database");
const app = express();

// Create variables for MQTT use here
const address = "mqtt://localhost:1883"; // address of the Lorix IP MQTT broker
const topic = "application/device/rx"; // add the pluses between the slashes application/+/device/+/rx
const publishTopic = "application/178/device/2cf7f1203230e7b3/tx";

app.use(bodyParser.json());
app.use(express.urlencoded({ extended: false }));

app.use(express.static(path.join(__dirname, "public")));

// create an MQTT instance
const client = mqtt.connect(address); // connecting to mqtt broker

// Check that you are connected to MQTT and subscribe to a topic (connect event)
client.on("connect", () => {
  //when connect
  client.subscribe(topic);
  console.log("connected to MQTT"); // prints that it connects to MQTT
});

// handle instance where MQTT will not connect (error event)
client.on("error", (error) => {
  console.log("Error: ", error);
});

client.on("message", (topic, message) => {
  try {
    // Parse the JSON message
    const parsedMessage = JSON.parse(message.toString());

    // Extract the base64 encoded data
    const base64Data = parsedMessage.data;

    // Decode the base64 data
    const decodedData = Buffer.from(base64Data, "base64").toString("utf-8");

    // Extract the relevant fields
    const currentTime = new Date().toLocaleString();
    console.log("Current Time:", currentTime);

    const extractedData = {
      data: parsedMessage.data, // TODO: Replace with decodedData
      time: currentTime,
    };

    // Log the extracted data
    console.log("Extracted Data:", extractedData);

    // Check if the data contains BATT=
    if (extractedData.data.includes("BATT=")) {
      // Insert battery status into battery column
      const rawValue = parseInt(extractedData.data.split("BATT=")[1], 10);
      const batteryPercent = convertAdcRawToBatteryPercent(rawValue);
      console.log(`Battery Percentage: ${batteryPercent}%`);
      db.run(
        "INSERT INTO battery (battery) VALUES (?)",
        [batteryPercent],
        function (err) {
          if (err) {
            return console.error(
              "Error inserting into activity table:",
              err.message
            );
          }
          console.log(`Inserted battery record with ID: ${this.lastID}`);
        }
      );
    } else {
      // Insert locked/unlocked status to status column
      db.run(
        "INSERT INTO activity (timestamp, status) VALUES (?, ?)",
        [extractedData.time, extractedData.data],
        function (err) {
          if (err) {
            return console.error(
              "Error inserting into activity table:",
              err.message
            );
          }
          console.log(`Inserted activity record with ID: ${this.lastID}`);
        }
      );
    }
  } catch (error) {
    console.error("Error processing MQTT message:", error.message);
  }
});

// Route to serve the home page
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "views/index.html"));
});

app.get("/logs", (req, res) => {
  res.sendFile(path.join(__dirname, "views/logs.html"));
});

app.get("/sensor-data", (req, res) => {
  db.all(
    "SELECT timestamp, status FROM activity ORDER BY rowid DESC LIMIT 500",
    [],
    (err, rows) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      res.json({ data: rows });
    }
  );
});

app.get("/latest-state", (req, res) => {
  db.get(
    "SELECT status FROM activity ORDER BY rowid DESC LIMIT 1",
    [],
    (err, activityRow) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      db.get(
        "SELECT battery FROM battery ORDER BY rowid DESC LIMIT 1",
        [],
        (err, batteryRow) => {
          if (err) {
            res.status(500).json({ error: err.message });
            return;
          }
          res.json({
            state: activityRow ? activityRow.status : "UNKNOWN",
            battery: batteryRow ? batteryRow.battery : "UNKNOWN",
          });
        }
      );
    }
  );
});

app.post("/send-command", (req, res) => {
  const { data } = req.body;
  const message = JSON.stringify({
    confirmed: false,
    fPort: 10,
    data: data,
  });

  client.publish(publishTopic, message, (err) => {
    if (err) {
      console.error("Error publishing MQTT message:", err.message);
      return res.status(500).json({ message: "Error sending command" });
    }
    res.json({ message: "Command sent successfully" });
  });
});

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});

function convertAdcRawToBatteryPercent(raw) {
  const BATTERY_MIN_VOLTAGE = 6.0;
  const BATTERY_MAX_VOLTAGE = 12.0;

  // Convert raw to approximate voltage
  const volts = (raw / 1023.0) * 3.3 * 11.0;

  // Map [6.0..12.0 V] to [0..100%]
  let percent;
  if (volts <= BATTERY_MIN_VOLTAGE) {
    percent = 0.0;
  } else if (volts >= BATTERY_MAX_VOLTAGE) {
    percent = 100.0;
  } else {
    percent =
      ((volts - BATTERY_MIN_VOLTAGE) /
        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) *
      100.0;
  }

  // Round to two decimal places
  return Math.round(percent * 100) / 100;
}
