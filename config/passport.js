const LocalStrategy = require("passport-local").Strategy;
const bcrypt = require("bcrypt");
const passport = require("passport");

const saltRounds = 10;
const plainPassword = "admin";

const users = [
  {
    id: 1,
    username: "admin",
    password: "$2b$10$CHI0j4.GsHbyV6k/0WxjGudMm/1Wh9LZerJVYaKtaDsTQRmrEO7sO",
  },
];

passport.use(
  new LocalStrategy(function (username, password, done) {
    const user = users.find((u) => u.username === username);
    if (!user) {
      return done(null, false, { message: "Incorrect username." });
    }
    bcrypt.compare(password, user.password, (err, res) => {
      if (res) {
        return done(null, user);
      } else {
        return done(null, false, { message: "Incorrect password." });
      }
    });
  })
);

// Serialize user instances to the session.
passport.serializeUser((user, done) => {
  done(null, user.id);
});

// Deserialize user instances from the session.
passport.deserializeUser((id, done) => {
  const user = users.find((u) => u.id === id);
  done(null, user);
});

module.exports = passport;
