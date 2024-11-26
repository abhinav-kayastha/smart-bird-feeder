const express = require("express");
const path = require("path");
const db = require("./config/database");
const app = express();

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

app.get("/activity", (req, res) => {
  res.sendFile(path.join(__dirname, "activity.html"));
});

app.get("/sensor-data", (req, res) => {
  db.all("SELECT value, status FROM activity", [], (err, rows) => {
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
