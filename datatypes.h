enum States {
  idle      = 0,
  active    = 1,
  startCapture = 2,
  capturing = 3,
  selectTeam = 4,
  changeOwner = 5,
  enterCalibration = 6,
  verifyCalibration = 7,
  verifySucess = 8,
  verifyFail = 9
};

enum Owner {
  neutral = 0,
  team1	= 1,
  team2	= 2,
  team3	= 3,
  team4 = 4
};