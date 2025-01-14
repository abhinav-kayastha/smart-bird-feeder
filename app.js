const express = require("express");
const path = require("path");
const passport = require("./config/passport");
const db = require("./config/database");
const session = require("express-session");
const app = express();

// Middleware for parsing request bodies
app.use(express.urlencoded({ extended: false }));

// Initialize session middleware
app.use(
  session({
    secret: "your_secret_key",
    resave: false,
    saveUninitialized: false,
  })
);

// Initialize Passport and restore authentication state, if any, from the session
app.use(passport.initialize());
app.use(passport.session());

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "views/index.html"));
});

app.get("/activity", ensureAuthenticated, (req, res) => {
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

// Login route
app.post(
  "/login",
  passport.authenticate("local", {
    successRedirect: "/activity",
    failureRedirect: "/",
  })
);

function ensureAuthenticated(req, res, next) {
  if (req.isAuthenticated()) {
    return next();
  }
  res.redirect("/");
}

app.listen(3000, () => {
  console.log("Server is running on port 3000");
});
