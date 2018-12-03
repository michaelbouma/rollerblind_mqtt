// ##################################################
// ##     Things to do when running standalone     ##
// ##################################################

void standaloneRun (void) {
  if (standalone==false)
    if (openhabTimeElapsed > heartbeat_timeout)
      standalone = true;

  if (standalone==true) // We lost connection with openhab, we are running on our own.
  {
  }
}
