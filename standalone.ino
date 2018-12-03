// ##################################################
// ##     Things to do when running standalone     ##
// ##################################################

void standaloneRun (void) {
  if (standalone==false)
  {
    if ((openhabTimeElapsed > 500) && (openhabTimeElapsed < 550))
      setColor(0,10,0); //color is green
    if (openhabTimeElapsed > heartbeat_timeout)
      standalone = true;
  }

  if (standalone==true) // We lost connection with openhab, we are running on our own.
  {
  }
}
