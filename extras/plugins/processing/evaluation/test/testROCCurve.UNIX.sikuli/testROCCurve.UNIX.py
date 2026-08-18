def setUp(self):
    import os
    import shutil
    ov_binany_path=os.environ['OV_BINARY_PATH']
    self.terminal = App.open("xterm -e " + ov_binany_path +"/designer --no-session-management --play-fast Test_ROCCurve.xml") # NOSONAR
    while not self.terminal.window():
        wait(1) # NOSONAR
        
 #def takepicture:
  
  #dir = os.path.dirname(getBundlePath()) # the folder, where your script is stored
  #img = capture(SCREEN) # snapshots the screen
  #shutil.move(img, os.path.join(dir, "shot.png")) 
  
def testROCCurve(self):
    import os
    import shutil
    try:
      wait("ROCCurveResult.png",60) # NOSONAR
      assert(exists("ROCCurveResult.png")) # NOSONAR
    except (FindFailed, AssertionError): # NOSONAR
      print("Unable to find the required png")
      # the folder, where your script is stored
      dir = os.path.dirname(getBundlePath()) # NOSONAR
      # snapshots the screen
      img = capture(SCREEN) # NOSONAR
      shutil.move(img, os.path.join(dir, "screenshot.png")) 
      raise
    
def tearDown(self):
    self.terminal.close()
