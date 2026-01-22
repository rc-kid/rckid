static  void outputHigh(Pin p) { write(p, true); setAsOutput(p); write(p, true);  }
static  void outputLow(Pin p) { write(p, false); setAsOutput(p); write(p, false); }
static  void outputFloat(Pin p) { setAsInput(p); }

static void high(Pin p) { write(p, true); }
static void low(Pin p) { write(p, false); }
