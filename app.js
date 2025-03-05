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
      data: parsedMessage.data,
      time: new Date().toISOString(),
    };

    // Log the extracted data
    console.log("Extracted Data:", extractedData);

    // Insert extracted data into 'time' table
    db.run(
      "INSERT INTO activity (time) VALUES (?)",
      [extractedData.time],
      function (err) {
        if (err) {
          return console.error("Error inserting into time table:", err.message);
        }
        console.log(`Inserted time record with ID: ${this.lastID}`);
      }
    );

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

// Route to serve the JSON array from the file message.json when requested from the home page
app.get("/messages", async (req, res) => {
  const messages = await read();
  res.json(messages);
});

// Route to serve the page to add a message
app.get("/add", (req, res) => {
  res.sendFile(path.resolve(__dirname, "./message.html"));
});

// Route to show a selected message. Note, it will only show the message as text. No html needed
app.get("/:id", async (req, res) => {
  const messages = await read(); // reads the current messages
  const message = messages.find((m) => m.id === req.params.id);
  res.send(message);
});

// Route to CREATE a new message on the server and publish to mqtt broker
app.post("/", async (req, res) => {
  const newMessage = { id: new Date().getTime().toString(), msg: req.body.msg };
  const messages = await read();
  messages.push(newMessage);
  await write(messages);
  client.publish(req.body.topic || topic, newMessage.msg);
  res.sendStatus(200);
});

// Route to delete a message by id
app.delete("/:id", async (req, res) => {
  try {
    const messages = await read();
    await write(messages.filter((c) => c.id !== req.params.id));
    res.sendStatus(200);
  } catch (e) {
    res.sendStatus(500);
  }
});

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});
