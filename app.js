const express = require("express");
const mqtt = require("mqtt");
const path = require("path");
const bodyParser = require("body-parser");
const db = require("./config/database");
const app = express();

// Create variables for MQTT use here
const address = "mqtt://192.168.50.192:1883";
const topic = "application/+/device/+/rx";

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
  console.log("Received message:", message.toString());
  try {
    // Parse the JSON message
    const parsedMessage = JSON.parse(message.toString());

    // Extract the base64 encoded data
    const base64Data = parsedMessage.data;

    // Decode the base64 data
    const decodedData = Buffer.from(base64Data, "base64").toString("utf-8");

    // Extract the relevant fields
    const extractedData = {
      data: decodedData,
      time: new Date().toISOString(),
    };

    // Log the extracted data
    console.log("Extracted Data:", extractedData);

    // Check if the data contains BATT=
    if (extractedData.data.includes("BATT=")) {
      // Insert battery status into battery column
      db.run(
        "INSERT INTO battery (battery) VALUES (?)",
        [extractedData.data],
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

app.get("/activity", (req, res) => {
  res.sendFile(path.join(__dirname, "views/activity.html"));
});

app.get("/sensor-data", (req, res) => {
  db.all("SELECT timestamp, status FROM activity", [], (err, rows) => {
    if (err) {
      res.status(500).json({ error: err.message });
      return;
    }
    res.json({ data: rows });
  });
});

app.get("/latest-state", (req, res) => {
  db.get(
    "SELECT status FROM activity ORDER BY timestamp DESC LIMIT 1",
    [],
    (err, activityRow) => {
      console.log(activityRow);
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      db.get(
        "SELECT battery FROM battery ORDER BY rowid DESC LIMIT 1",
        [],
        (err, batteryRow) => {
          console.log(batteryRow);
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

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});
