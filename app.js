const express = require("express");
const mqtt = require("mqtt");
const path = require("path");
const bodyParser = require("body-parser");
const db = require("./config/database");
const app = express();

// Create variables for MQTT use here
const address = "mqtt://localhost:1883";
const topic = "iot/timestamp_status";

app.use(bodyParser.json());
app.use(express.urlencoded({ extended: false }));

app.use(express.static(path.join(__dirname, "public")));

// create an MQTT instance
const client = mqtt.connect(address, {
  clientId: "abhinav",
  username: "SmartIoT",
  password: "SmartIoTMQTT",
}); // connecting to mqtt broker, using the clientId: abhinav

// Check that you are connected to MQTT and subscribe to a topic (connect event)
client.on("connect", () => {
  //when connect
  client.subscribe(topic); // then subscribes to topic abhinav
  console.log("connected to MQTT"); // prints that it connects to MQTT
});

// handle instance where MQTT will not connect (error event)
client.on("error", (error) => {
  console.log("Error: ", error);
});

// Handle when a subscribed message comes in (message event)
client.on("message", (topic, message) => {
  try {
    // Parse the JSON message
    const parsedMessage = JSON.parse(message.toString());

    // Extract the relevant fields
    const extractedData = {
      data: parsedMessage.status,
      time: new Date().toISOString(),
    };

    // Log the extracted data
    console.log("Extracted Data:", extractedData);

    // Insert extracted data into 'activity' table
    db.run(
      "INSERT INTO activity (time, status) VALUES (?, ?)",
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
  db.all("SELECT time, status FROM activity", [], (err, rows) => {
    if (err) {
      res.status(500).json({ error: err.message });
      return;
    }
    res.json({ data: rows });
  });
});

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});
