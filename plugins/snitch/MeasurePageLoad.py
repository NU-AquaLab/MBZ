import json
from urlparse import urlparse
import os
import time
from subprocess import call, Popen
import requests
from PIL import Image
import multiprocessing
import websocket
#from timeout import timeout

# debugging purposes
LOGSENTMSGS = True
LOGRESPONSES = True

def attemptConnection(remote_debug_url):
    didSucceed = False
    num_tries = 10
    for i in range(0, num_tries):
        try:
            r = requests.get(remote_debug_url)
        except requests.ConnectionError:
            pass
        else:
            didSucceed = True
            break

    if didSucceed:
        return r
    else:
        print("Connection failed "+str(num_tries)+" times.  This could be due to Chrome taking longer than usual to open.")
        resp = raw_input("Would you like to keep trying to connect? (y or n)\n>")
        if resp.lower() == 'y':
            return "try_again"
        elif resp.lower() == 'n':
            return "give_up"
        else:
            print("Invalid response.  We will try connecting again.")
            return "try_again"

def getTabNumber(resp_json):
    print("\nThere are "+str(len(resp_json)) + " available tabs:")
    i = 0
    for this_tab in resp_json:
        try:
            page_title = str(this_tab["title"])
        except:
            page_title = "ERR: unable to read page title"
        print(str(i)+": "+page_title)
        i += 1
    tab_number = input("\nWhich tab would you like to drive remotely (0-"+str(len(resp_json)-1)+")?\n"+
                        "(Recommendation: Choose the tab that is active)\n>")
    return tab_number

def getTabsJSON(debug_port):
    remote_debug_url = "http://localhost:"+str(debug_port)+"/json"
    while True:
        print("Opening websocket remote debugging connection to "+remote_debug_url)
        r = attemptConnection(remote_debug_url)
        if r == "try_again":
            pass
        elif r == "give_up":
            resp_json = {}
            shouldQuit = True
            return resp_json, shouldQuit
        else:
            break
    resp_json = r.json()
    shouldQuit = False
    return resp_json, shouldQuit

def getScreenDimensions():
    # print("\nPlease provide the screen dimensions of your phone.")
    # print("(This is needed for passing input events such as screen taps)")
    # screen_width = input("screen width>")
    # screen_height = input("screen height>")
    screen_width = 100
    screen_height = 100
    return screen_width, screen_height

def shouldContinue():
    resp = raw_input("Would you like to continue? (y or n)\n>")
    if resp.lower() == 'y':
        return True
    elif resp.lower() == 'n':
        return False
    else:
        print("Invalid response.  Please try again.")
        return shouldContinue()

def getLocation():
    # return raw_input("\nWhat is your location?\n(e.g. 'home'. Needed for filename convention)\n>")
    return "home"

def connectToDevice(debug_port):
    '''
    Asks the user which device to drive.  Makes a command line call to connect to that device.
    '''
    print("")
    op_sys = getUserOS()
    # choice_num = input("\nDo you want to drive Chrome on your phone or on your computer?\n"+
    #                     "1. Chrome on phone\n"+
    #                     "2. Chrome on computer\n>")
    choice_num = 1
    if choice_num == 1:
        device_type = "phone"
        # command = "adb forward tcp:"+str(debug_port)+" localabstract:chrome_devtools_remote"
        command = ["adb", "forward", "tcp:"+str(debug_port), "localabstract:chrome_devtools_remote"]
        ret_code = printAndCall(command)
        if ret_code == 1:
            print("Error.  Please ensure that phone is connected and try again.")
            return connectToDevice(debug_port)
    elif choice_num == 2:
        device_type = "computer"
        if shouldOpenChromeCanary():
            openChromeCanary(debug_port, op_sys)
    else:
        print("ERR: Invalid choice.")
        raise
    device_name = getDeviceName()
    return device_name, device_type, op_sys

def fixURL(url):
    if not urlHasScheme(url):
        return "http://"+url
    else:
        return url

def urlHasScheme(url):
    if url[0:4] == "http":
        return True
    else:
        return False

def getDelayAndBandwidth(device_type):
    if device_type == "computer":
        using_NLC = raw_input("Are you using the Network Link Conditioner to control latency and/or bandwidth?\n>")
        if using_NLC.lower() == 'y' or using_NLC.lower() == 'yes':
            delay_NLC = raw_input("Enter the Delay in ms\n>")
            if delay_NLC == "" or delay_NLC == "None":
                delay_NLC = None
            else:
                delay_NLC = float(delay_NLC)

            bandwidth_NLC = raw_input("Enter the Bandwidth in Kbps\n>")
            if bandwidth_NLC == "" or bandwidth_NLC == "None":
                bandwidth_NLC = None
            else:
                bandwidth_NLC = float(bandwidth_NLC)

        elif using_NLC.lower() == 'n' or using_NLC.lower() == 'no':
            delay_NLC = None
            bandwidth_NLC = None
        else:
            print("Invalid response.  Try again.")
            return getDelayAndBandwidth(device_type)
    else:
        delay_NLC = None
        bandwidth_NLC = None

    return delay_NLC, bandwidth_NLC


def getNetworkType(device_type):
    if device_type == "phone":
        valid_resp_list = ["wifi", "4g", "3g"]
    elif device_type == "computer":
        valid_resp_list = ["wifi", "wired", "tether_4g", "tether_3g"]
    else:
        print("ERR. Invalid device_type: "+str(device_type))
        raise

    # ask_str = "What type of network are you connected to?\nValid responses are: "
    # for resp in valid_resp_list:
    #     ask_str += (resp+", ")
    ask_str = "wifi"
    network_type = raw_input(ask_str+"\n>")
    network_type = network_type.lower()
    if network_type in valid_resp_list:
        return network_type
    else:
        print("Invalid response.  Please try again.")
        return getNetworkType(device_type)

def openChromeCanary(debug_port, op_sys):
    path_to_chrome = _getChromePath()
    user_data_dir = _getUserDataDir(op_sys)
    # command = '"' + path_to_chrome + '" --remote-debugging-port='+str(debug_port)
    # if user_data_dir != "":
    #     command = command +  ' --user-data-dir="'+str(user_data_dir)+'"'
    command = []
    rdp_flag = "--remote-debugging-port="+str(debug_port)
    if op_sys == "mac" or op_sys == "linux":
        path_to_chrome = path_to_chrome.replace('\ ', '____')
        path_to_chrome = path_to_chrome.split(' ')
        path_to_chrome = [elem.replace('____', ' ') for elem in path_to_chrome]
        command = path_to_chrome
        command.append("--args")
        command.append(rdp_flag)
        print(str(path_to_chrome))
        print(str(command))
    else:
        command = [path_to_chrome, rdp_flag]

    print(str(command))
    if user_data_dir != "":
        command.append("--user-data-dir="+str(user_data_dir))
    print('Popening '+str(command))
    p_chrome = Popen(command)

def shouldOpenChromeCanary():
    resp = raw_input("Is Chrome already open with --remote-debugging-port flag?\n"+
                    "(Enter y or n)\n>")
    if resp == 'y':
        return False
    elif resp == 'n':
        return True
    else:
        print("Invalid response.  Please enter y or n")
        return shouldOpenChromeCanary()

def _getChromePath():
    '''
    Asks the user to provide the path to the Chrome Canary executable.
    Returns a string representing the full path.
    '''
    path_to_chrome = raw_input("Enter the path to your Chrome Canary executable, or choose one of the options below.\n"+
                            "0. 'C:\Users\[username]\AppData\Local\Google\Chrome SxS\Application\chrome.exe' (normal path to Chrome Canary on Windows)\n"+
                            "1. 'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe' (normal path to Chrome on Windows)\n"+
                            "2. 'open -a Google\ Chrome' (command to open Google Chrome on Mac)\n>")
    if path_to_chrome == '0':
        #username = raw_input("Enter your username\n>")
        username = os.environ['USERNAME']
        path_to_chrome = os.path.join("C:\Users", username, "AppData\Local\Google\Chrome SxS\Application\chrome.exe")
    elif path_to_chrome == '1':
        # chrome or chrome beta
        path_to_chrome = "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"
    elif path_to_chrome == '2':
        path_to_chrome = "open -a Google\ Chrome"
    return path_to_chrome

def _getUserDataDir(op_sys):
    user_data_dir = raw_input("Enter the path to the Chrome --user-data-dir you wish to use.\n"+
                            "This should be different than the default user data directory.\n>")
    if user_data_dir == 'a':
        # easter egg for me
        if op_sys == "windows":
            user_data_dir = "C:\Users\Andrew\OneDrive\Documents\Northwestern\Research\web_ads\chrome-profile"
        else:
            user_data_dir = "/Users/andrew/Documents/Northwestern/Research/web-ads/chrome-profile"

    return user_data_dir


def printAndCall(command):
    print("Calling '"+str(command)+"'")
    return call(command)

def getDeviceName():
    # device_name = raw_input("What do you want to name this device? (e.g. \"MacBookPro\" or \"GalaxyS5\")\n"+
    #                     "(This is used for file naming conventions.  No spaces or '-' permitted.)\n"+
    #                     ">")
    device_name = "abc"
    if '-' in device_name or ' ' in device_name:
        print("This device name is invalid.  Try again.")
        return getDeviceName()
    else:
        return device_name

def getUserOS():
    # op_sys = raw_input("What OS are you running this script on?\n"+
    #                     "1. Windows\n"+
    #                     "2. Mac OS\n"+
    #                     "3. Linux (untested)\n>")
    op_sys = '2'
    if op_sys == '1':
        return "windows"
    elif op_sys == '2':
        return "mac"
    elif op_sys == '3':
        return "linux"
    else:
        print("Invalid selection.")
        return getUserOS()

def getMeasureTarget():
    measure = raw_input("What would you like to measure?\n"+
                        "1. Ads\n"+
                        "2. Ad-blocker itself\n>")
    if measure == '1':
        return "ads"
    elif measure == '2':
        return "ad-blocker"
    else:
        print("Invalid selection.")
        return getMeasureTarget()


class MeasurePageLoad:
    """
    MeasurePageLoad is a class for running page load measurements over websockets
    using Google Chrome's Remote Debugging feature.
    """
    
    # constructor
    def __init__(self, url_ws, page_url = None, cutoff_time = None, device_name="no_name", device_type = "computer", delay_NLC=None,
                bandwidth_NLC=None, debug_port=9222, phone_adBlockPackage="com.savageorgiev.blockthis", network_type=None, location=None,
                 output_dir=None, op_sys=None, start_time=None, min_time=None, screen_width=None, screen_height=None, measure="ads"):
        self.url_ws = url_ws                 # url to use for the websocket.WebSocket object
        self.ws = None                      # websocket.Websocket object
        self.msg_list = []             # raw data - list of debug messages received from ws
        self.id = 1                     # initialize to 1
        self.phone_adBlockPackage = phone_adBlockPackage
        self.location = location
        self.device_name = device_name
        self.device_type = device_type           # "phone" or "computer"
        self.delay_NLC = delay_NLC
        self.bandwidth_NLC = bandwidth_NLC
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.measure = measure
        self.op_sys = op_sys
        self.network_type = network_type
        self.start_time = start_time
        self.min_time = min_time
        self.sample_num = None
        self.hostname = None
        self.page_url = page_url
        self.cutoff_time = cutoff_time
        self.debug_port = debug_port
        self.categories_and_ranks = None    # The Alexa categories this URL is ranked in
        self.wsOverhead = 0
        self.loadPageAndSaveDataTime = 0

        #self.p = None
        self.frames = {}
        self.firstFrame = False
        self.logMsgs = []
        self.rawData_fname = None
        self.log_fname = str(time.time())+".log"
        self.summary_fname = None
        self.output_dir = output_dir

        self.domContentEventFired = False
        self.onLoadFired = False
        self.reachedCutoffTime = False
        self.timings = {'first_timestamp': None}
        # self.first_timestamp = None
        # self.last_timestamp = None
        # self.dom_timestamp = None
        # self.onload_timestamp = None
        self.statsList = ["PageEventCount", "NetworkEventCount", "numObjsRequested", "cumulativeRequestSize", "requestServedFromCacheCount", "responseReceivedCount", "numDataChunksReceived", "cumulativeDataLength", "cumulativeEncodedDataLength", "loadingFinishedCount", "cumulativeEncodedDataLength_LF", "loadingFailedCount", "numBlockedExplicitly", "frameAttachedCount", "frameNavigatedCount", "frameDetachedCount", "frameStartedLoadingCount", "frameStoppedLoadingCount", "frameScheduledNavigationCount", "frameClearedScheduledNavigationCount", "frameResizedCount", "javascriptDialogOpeningCount", "javascriptDialogClosedCount", "screencastFrameCount", "screencastVisibilityChangedCount", "colorPickedCount", "interstitialShownCount", "interstitialHiddenCount", "otherPageEventCount", "otherNetworkEventCount"]
        self.stats = {}
        self.resetStatsDict()
        self.makeEmptyAttrDicts()
        self._isBlockingAds = self.isAdBlockEnabled()

    def makeEmptyAttrDicts(self):
        self.numObjsLoadedByStatus                  = {}
        self.numObjsLoadedByType                    = {}
        self.statsAtDOMEvent                        = {}
        self.statsAtOnLoadEvent                     = {}

    def resetStatsDict(self):
        for key in self.statsList:
            self.stats[key] = 0

    def writeDataSummaryToFile(self):
        print("Writing data summary to file: "+self.summary_fname)
        temp_dict = {}

        # meta data
        temp_dict['rawDataFile'] = self.rawData_fname
        temp_dict['logFile'] = self.log_fname
        temp_dict['location'] = self.location
        temp_dict['device_type'] = self.device_type
        temp_dict['networkType'] = self.network_type
        temp_dict['isBlockingAds'] = self._isBlockingAds
        temp_dict['sampleNumber'] = self.sample_num
        temp_dict['hostname'] = self.hostname
        temp_dict['pageURL'] = self.page_url
        temp_dict['cutoffTime'] = self.cutoff_time
        temp_dict['debugPort'] = self.debug_port
        temp_dict['categories_and_ranks'] = self.categories_and_ranks
        temp_dict['reachedCutoffTime'] = self.reachedCutoffTime

        # timings
        for key in self.timings:
            temp_dict[key] = self.timings[key]
        
        try:
            temp_dict['time_DOMContent'] = self.timings['dom_timestamp'] - self.timings['first_timestamp']
        except (TypeError, KeyError):
            temp_dict['time_DOMContent'] = None

        try:
            temp_dict['time_onLoad'] = self.timings['onload_timestamp'] - self.timings['first_timestamp']
        except (TypeError, KeyError):
            temp_dict['time_onLoad'] = None

        try:
            temp_dict['time_finishLoad'] = self.timings['last_timestamp'] - self.timings['first_timestamp']
        except (TypeError, KeyError):
            temp_dict['time_finishLoad'] = None

        # objects counts
        temp_dict['statsAtDOMEvent'] = self.statsAtDOMEvent
        temp_dict['statsAtOnLoadEvent'] = self.statsAtOnLoadEvent
        for key in self.stats:
            temp_dict[key] = self.stats[key]

        temp_dict['numObjsLoadedByStatus'] = self.numObjsLoadedByStatus
        temp_dict['numObjsLoadedByType'] = self.numObjsLoadedByType

        summary_path = os.path.join(self.output_dir, "summaries", self.summary_fname)
        with open(summary_path, 'w') as f:
            json.dump(temp_dict, f)


    def writeLog(self):
        print("Writing log file.")
        end_time = time.time()
        time_elapsed = end_time - self.start_time
        self.logMsgs.append("Theoretical time spent loading pages: "+str(self.min_time*60))
        self.logMsgs.append("Actual total time: "+str(time_elapsed))
        self.logMsgs.append("Ratio: "+str(time_elapsed/(self.min_time*60)))
        self.logMsgs.append("Web Socket create/close overhead: "+str(self.wsOverhead))
        self.logMsgs.append("Web Socket percent of total time: "+str(self.wsOverhead/time_elapsed))
        self.logMsgs.append("Load Page and Save Data time: "+str(self.loadPageAndSaveDataTime))
        self.logMsgs.append("Load Page and Save Data percent of total time: "+str(self.loadPageAndSaveDataTime/time_elapsed))

        log_dir = os.path.join(self.output_dir,"log")
        log_path = os.path.join(log_dir, self.log_fname)
        f = open(log_path, 'w')
        for msg in self.logMsgs:
            f.write(msg+"\n")
        f.close()

    def sendMethod(self, method_name, params, shouldPrintResp):
        if params == None:
            payload_dict = {'id': self.id, 'method': method_name}
        else:
            payload_dict = {'id': self.id, 'method': method_name, 'params': params}

        payload = json.dumps(payload_dict)
        print("sending: "+payload)
        self.ws.send(payload)
        
        if LOGSENTMSGS:
            self.msg_list.append(payload_dict)

        resp_obj = self._getNextWsResp(LOGRESPONSES)
        resp = str(resp_obj)
        self.id += 1

        if shouldPrintResp:
            if(len(resp) > 100):
                print("received: "+resp[:100]+"...")
            else:
                print("received: "+resp)

        return resp_obj

    def getRespMethod(self, resp_obj):
        try:
            method = resp_obj['method']
        except KeyError:
            method = None
        return method

    def getRespURL(self, resp_obj):
        try:
            url = resp_obj['params']['url']
        except KeyError:
            url = None
        return url

    def isAdBlockEnabled(self):
        if self.device_type == "computer":
            return self.isAdBlockEnabledComputer()
        elif self.device_type == "phone":
            print("Attempting to kill ad block on phone in case it's already running")
            self._disableAdBlockPhone()
            return False
            #return self.isAdBlockEnabledPhone()
        else:
            print("ERR. Unknown device_type '"+str(self.device_type)+"'.")
            raise

    def isAdBlockEnabledComputer(self):
        if self.measure == "ads":
            target_title = "Adblock Plus"
        elif self.measure == "ad-blocker":
            target_title = "Adblock QMinus"
        r = requests.get("http://localhost:"+str(self.debug_port)+"/json")
        resp_json = r.json()
        for this_tab in resp_json:
            if this_tab['title'] == target_title:
                return True

        # if Adblock Plus not found in json obj, then not enabled
        return False


    def clearAllCaches(self):
        self.clearBrowserObjCache()
        self.clearDeviceDNSCache()
        self.clearBrowserDNSCache()


    def enableAdBlock(self):
        if self.device_type == "computer":
            if self.isAdBlockEnabled() == False:
                print("Enabling ad-blocker")
                self._enableAdBlockComputer()
                if self.measure == "ads":
                    self._disableAdBlockMinus()
                print("Ad-blocker is now enabled")
            else:
                print("Ad-blocker is already enabled")
                return
        elif self.device_type == "phone":
            print("Enabling ad-blocker")
            self._enableAdBlockPhone()
            print("Ad-blocker is now enabled")
        else:
            print("ERR. Unknown device_type '"+self.device_type+"'")
            raise

        self._isBlockingAds = True

    def disableAdBlock(self):
        if self.device_type == "computer":
            self._disableAdBlockComputer()
            if self.measure == "ads":
                self._enableAdBlockMinus()
        elif self.device_type == "phone":
            self._disableAdBlockPhone()
        else:
            print("ERR. Unknown device_type '"+self.device_type+"'")
            raise

        self._isBlockingAds = False

    def _enableAdBlockMinus(self):
        # down arrow, then space bar
        down_arrow = "U+2193"
        space_bar = space_bar = "U+0020"
        #self.pressAndReleaseKey(down_arrow)
        self._tapDownArrow()
        self.pressAndReleaseKey(space_bar)

    def _tapDownArrow(self):
        #params = {'type': 'rawKeyDown', 'keyIdentifier': "Down", "isKeypad": False}
        params = {'type': 'keyDown', 'keyIdentifier': "Down", "isKeypad": False}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def _disableAdBlockMinus(self):
        # down arrow, then space bar
        down_arrow = "U+2193"
        space_bar = space_bar = "U+0020"
        #self.pressAndReleaseKey(down_arrow)
        self._tapDownArrow()
        self.pressAndReleaseKey(space_bar)


    def _disableAdBlockComputer(self):
        dst_url = "chrome://extensions/"
        self.loadURLandWait(dst_url)
        if self.op_sys == 'windows':
            num_tabs = 5
        else:
            num_tabs = 6

        # tab 5 times
        tab_key = "U+0009"
        #self.pressKeyKtimes(tab_key, num_tabs)
        tabKeyCode = 9
        self.pressVirtualKeyKTimes(tabKeyCode, num_tabs)

        # then space bar
        space_bar = "U+0020"
        self.pressAndReleaseKey(space_bar)

    def _disableAdBlockPhone(self):
        # command = "adb shell am force-stop "+self.phone_adBlockPackage
        command = ["adb", "shell", "am", "force-stop", self.phone_adBlockPackage]
        printAndCall(command)

    def _enableAdBlockComputer(self):
        dst_url = "chrome://extensions/"
        self.loadURLandWait(dst_url)
        if self.op_sys == "windows":
            num_tabs = 3
        else:
            num_tabs = 3

        # tab 3 times
        tab_key = "U+0009"
        #self.pressKeyKtimes(tab_key, num_tabs)
        tabKeyCode = 9
        self.pressVirtualKeyKTimes(tabKeyCode, num_tabs)

        # then space bar
        space_bar = "U+0020"
        self.pressAndReleaseKey(space_bar)

    def _enableAdBlockPhone(self):
        # first try to force-stop in case it's already running
        print("Killing any already running ad-blocker")
        self._disableAdBlockPhone()

        # then start the ad-blocker
        # command = "adb shell am start "+self.phone_adBlockPackage
        command = ["adb", "shell", "am", "start", self.phone_adBlockPackage]
        printAndCall(command)
        
        # then tap start button
        # screen_width = 720
        # screen_height = 1280
        tap_xcoord = self.screen_width / 3
        tap_ycoord = self.screen_height / 2
        # tap_xcoord = 525
        # tap_ycoord = 925
        self.waitForBlockThisStartBtn(tap_xcoord, tap_ycoord)
        # command = "adb shell input tap "+str(tap_xcoord)+" "+str(tap_ycoord)
        command = ["adb", "shell", "input", "tap", str(tap_xcoord), str(tap_ycoord)]
        printAndCall(command)

    def waitForBlockThisStartBtn(self, xcoord, ycoord):
        target_R = 119
        target_G = 179
        target_B = 212
        while True:
            if self.checkPixelColor(xcoord, ycoord, target_R, target_G, target_B):
                return

    def checkPixelColor(self, xcoord, ycoord, target_R, target_G, target_B):
        # save screencap
        command = "adb shell screencap /sdcard/screen.png"
        command = command.split(' ')
        printAndCall(command)

        # pull screencap to computer
        command = "adb pull /sdcard/screen.png"
        command = command.split(' ')
        printAndCall(command)

        im = Image.open("screen.png")
        pixels = im.load()
        target_pixel = pixels[xcoord, ycoord]
        if target_pixel[0] == target_R:
            if target_pixel[1] == target_G:
                if target_pixel[2] == target_B:
                    os.remove("screen.png")
                    return True

        os.remove("screen.png")
        return False

    def pressAndReleaseKey(self, keyIdentifier):
        params = {'type': 'keyDown', 'keyIdentifier': keyIdentifier}
        self.sendMethod("Input.dispatchKeyEvent", params, True)
        params = {'type': 'keyUp', 'keyIdentifier': keyIdentifier}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def pressAndReleaseVirtualKey(self, virtualKeyCode):
        params = {'type': 'rawkeyDown',
                  'nativeVirtualKeyCode': virtualKeyCode,
                  'windowsVirtualKeyCode': virtualKeyCode}
        self.sendMethod("Input.dispatchKeyEvent", params, True)
        params = {'type': 'keyUp',
                  'nativeVirtualKeyCode': virtualKeyCode,
                  'windowsVirtualKeyCode': virtualKeyCode}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def waitForPageLoad(self):
        while True:
            resp = self._getNextWsResp(LOGRESPONSES)
            try:
                method_name = str(resp["method"])
            except:
                pass
            else:
                #print("method: "+method_name)
                if method_name == "Page.loadEventFired":
                    time.sleep(0.5)
                    return

    def loadURLandWait(self, dst_url):
        self.navToURL(dst_url)
        self.waitForPageLoad()

    def clearBrowserObjCache(self):
        self.sendMethod("Network.setCacheDisabled", None, True)
        self.sendMethod("Network.clearBrowserCache", None, True)
        return

    def clearBrowserDNSCache(self):
        dst_url = "chrome://net-internals/#dns"
        self.loadURLandWait(dst_url)

        # tab 6 times
        tab_key = "U+0009"
        self.pressKeyKtimes(tab_key, 6)

        # press and release space bar
        space_bar = "U+0020"
        self.pressAndReleaseKey(space_bar)

        return

    def clearBrowserDNSCacheNotWorking(self):
        dst_url = "chrome://net-internals/#dns"
        self.loadURLandWait(dst_url)

        # Ctrl + F
        f_key = "U+0066"
        self.pressCtrlAndKey(f_key)

        # send 'clear host cache' (should go into search box)
        search_text = "clear host cache"
        self.dispatchText(search_text)

        # Escape key
        esc_key = "U+001B"
        self.pressAndReleaseKey(esc_key)

        # space bar
        space_bar = "U+0020"
        self.pressAndReleaseKey(space_bar)

    def dispatchTextOld(self, text):
        params = {'type': 'char', 'text': text}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def dispatchText(self, text):
        for this_char in text:
            hex_str = hex(ord(this_char))
            hex_full = hex_str.split("x",1)[1]
            while len(hex_full) < 4:
                hex_full = "0"+hex_full
            keyIdentifier = "U+"+hex_full
            self.pressAndReleaseKey(keyIdentifier)

    def pressCtrlAndKey(self, keyIdentifier):
        params = {'type': 'keyDown', 'modifiers': 2, 'keyIdentifier': keyIdentifier}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def pressAltAndKey(self, keyIdentifier):
        params = {'type': 'keyDown', 'modifiers': 1, 'keyIdentifier': keyIdentifier}
        self.sendMethod("Input.dispatchKeyEvent", params, True)

    def clearDeviceDNSCache(self):
        if self.device_type == "computer":
            self.clearComputerDNScache()
        elif self.device_type == "phone":
            self.clearPhoneDNScache()
        else:
            print("ERR. Unknown device_type '"+str(self.device_type)+"'")
            raise
        return

    def clearComputerDNScache(self):
        if self.op_sys == 'windows':
            command = "ipconfig /flushdns"
            printAndCall(command)
        elif self.op_sys == 'linux':
            command = ["service", "nscd", "restart"]
            printAndCall(command)
        elif self.op_sys == 'mac':
            command = "sudo dscacheutil -flushcache"
            command = command.split(' ')
            printAndCall(command)

            command = "sudo killall -HUP mDNSResponder"
            command = command.split(' ')
            printAndCall(command)

    def clearPhoneDNScache(self):
        #print("Toggling Airplane Mode")
        #self.toggleAirplaneMode()
        return

    def toggleAirplaneMode(self):
        sleep_time = 0.5

        # open settings window
        command = "adb shell am start -a android.settings.AIRPLANE_MODE_SETTINGS"
        command = command.split(' ')
        printAndCall(command)
        time.sleep(sleep_time)

        # tap on airplane mode
        self.adbShellKeyEvent("KEYCODE_DPAD_UP")
        time.sleep(sleep_time)
        self.adbShellKeyEvent("KEYCODE_ENTER")
        time.sleep(sleep_time)

        # tap on off/on button twice
        self.adbShellKeyEvent("KEYCODE_DPAD_RIGHT")
        time.sleep(sleep_time)
        self.adbShellKeyEvent("KEYCODE_ENTER")
        time.sleep(sleep_time*4)
        self.adbShellKeyEvent("KEYCODE_ENTER")
        time.sleep(sleep_time*4)

    def adbShellKeyEvent(self, keycode):
        command = "adb shell input keyevent "+keycode
        command = command.split(' ')
        printAndCall(command)

    def pressKeyKtimes(self, keyIdentifier, num_times):
        #params = {'type': 'rawKeyDown', 'keyIdentifier': keyIdentifier}
        params = {'type': 'keyDown', 'keyIdentifier': keyIdentifier}
        for i in range(0, num_times):
            self.sendMethod("Input.dispatchKeyEvent", params, True)

    def pressVirtualKeyKTimes(self, virtualKeyCode, num_times):
        params = {'type': 'rawkeyDown',
                  'nativeVirtualKeyCode': virtualKeyCode,
                  'windowsVirtualKeyCode': virtualKeyCode}
        for i in range(0, num_times):
            self.sendMethod("Input.dispatchKeyEvent", params, True)


    def navToURL(self, dst_url):
        if LOGSENTMSGS == False and LOGRESPONSES == False:
            self.msg_list = []      # first reset the message list

        if self.device_type == "phone":
            # bring chrome to front so we can watch
            command = "adb shell am start com.android.chrome"
            command = command.split(' ')
            printAndCall(command)

        params = {'url': dst_url}
        resp = self.sendMethod('Page.navigate', params, True)
        return resp

    def _updateFrameDict(self, resp):
        try:
            method = resp["method"]
        except:
            pass
        else:
            if method == "Page.frameStartedLoading":
                frameId = resp["params"]["frameId"]
                #print(resp)
                self.frames[frameId] = True
                self.firstFrame = True
            elif method == "Page.frameStoppedLoading":
                frameId = resp["params"]["frameId"]
                #print(resp)
                try:
                    self.frames.pop(frameId)
                except KeyError:
                    log_msg = "frameStoppedLoading KeyError: "+self.rawData_fname+" "+str(resp)
                    self.logMsgs.append(log_msg)

                    # print(resp)
                    # self.writeRawDataToFile()
                    # self.writeLog()
                    # self.writeDataSummaryToFile()
                    # raise


    #@timeout(3)
    def _getNextWsResp(self, shouldLog):
        '''
        Returns a json object representing the most recent response.
        '''
        resp = json.loads(self.ws.recv())
        if shouldLog:
            self.msg_list.append(resp)
        #conn.send(resp)
        return resp

    def _getFirstTimestamp(self):
        """
        Finds the first message with a valid timestamp.  Saves this value to 
        the object's first_timestamp attribute and returns the value.
        """
        print("Getting first_timestamp")
        while True:
            first_resp = self._getNextWsResp(True)  # returns json obj

            try:
                method = first_resp["method"]
            except:
                method = None
                pass

            # find first requestWillBeSent event
            if method == "Network.requestWillBeSent":
                try:
                    first_timestamp = first_resp["params"]["timestamp"]
                except KeyError as e:
                    print("KeyError: "+str(first_resp))
                else:
                    # if successfully got first_timestamp, break out of loop
                    break

        print("first_timestamp: " + str(first_timestamp))
        self.timings['first_timestamp'] = first_timestamp
        return first_timestamp

    def _isPastCutOffTime(self):
        if (self.timings['last_timestamp'] - self.timings['first_timestamp']) > self.cutoff_time:
            return True

    def _timeout(self):
        self.reachedCutoffTime = True
        print("Reached cutoff_time.  last_timestamp: " + str(self.timings['last_timestamp']))

    def _finishedLoading(self):
        # print("Nothing received through websocket for a few seconds.  Assume page is finished loading.")
        # print("Last resp recieved:")
        # print(this_resp)
        # print("last_timestamp: "+str(this_timestamp))
        # self.last_timestamp = this_timestamp
        if self.firstFrame == True and len(self.frames) == 0 and self.domContentEventFired and self.onLoadFired:
            return True

    def enforceNoneBlocked(self, resp):
        try:
            errorText = resp["params"]["errorText"]
        except:
            pass
        else:
            if errorText == "net::ERR_BLOCKED_BY_CLIENT" or errorText == "net::ERR_CONNECTION_REFUSED":
                self.logMsgs.append("Obj blocked while ad-blocker disabled: "+self.rawData_fname+" "+errorText)

    def enforceNoCache(self, resp):
        try:
            method = resp["method"]
        except:
            pass
        else:
            if method == "Network.requestServedFromCache":
                self.logMsgs.append("Obj served from cache: "+self.rawData_fname+" "+str(resp))

    def validateResp(self, resp):
        if self._isBlockingAds == False:
            self.enforceNoneBlocked(resp)
        self.enforceNoCache(resp)

    def processResp(self, resp):
        self.validateResp(resp)
        self._updateFrameDict(resp)
        try:
            method = resp['method']
        except KeyError:
            method = None
            self.handleRespNoneMethod(resp)
        else:
            self.handleRespMethod(resp, method)

        return method

    def handleRespPage(self, resp, method):
        self.stats['PageEventCount'] += 1
        if method == "Page.domContentEventFired":
            self.timings['dom_timestamp'] = resp['params']['timestamp']
            self.domContentEventFired = True
            self.statsAtDOMEvent['NetworkEventCount'] = self.stats['NetworkEventCount']
            self.statsAtDOMEvent['numObjsRequested'] = self.stats['numObjsRequested']
            self.statsAtDOMEvent['cumulativeRequestSize'] = self.stats['cumulativeRequestSize']
            self.statsAtDOMEvent['requestServedFromCacheCount'] = self.stats['requestServedFromCacheCount']
            self.statsAtDOMEvent['numObjsLoadedByStatus'] = self.numObjsLoadedByStatus
            self.statsAtDOMEvent['responseReceivedCount'] = self.stats['responseReceivedCount']
            self.statsAtDOMEvent['numObjsLoadedByType'] = self.numObjsLoadedByType
            self.statsAtDOMEvent['numDataChunksReceived'] = self.stats['numDataChunksReceived']
            self.statsAtDOMEvent['cumulativeDataLength'] = self.stats['cumulativeDataLength']
            self.statsAtDOMEvent['cumulativeEncodedDataLength'] = self.stats['cumulativeEncodedDataLength']
            self.statsAtDOMEvent['loadingFinishedCount'] = self.stats['loadingFinishedCount'] # BEWARE these events seem to get logged well after timestamp
            self.statsAtDOMEvent['cumulativeEncodedDataLength_LF'] = self.stats['cumulativeEncodedDataLength_LF']
            self.statsAtDOMEvent['loadingFailedCount'] = self.stats['loadingFailedCount']
            self.statsAtDOMEvent['numBlockedExplicitly'] = self.stats['numBlockedExplicitly']
        elif method == "Page.loadEventFired":
            self.timings['onload_timestamp'] = resp['params']['timestamp']
            self.onLoadFired = True
            self.statsAtOnLoadEvent['NetworkEventCount'] = self.stats['NetworkEventCount']
            self.statsAtOnLoadEvent['numObjsRequested'] = self.stats['numObjsRequested']
            self.statsAtOnLoadEvent['cumulativeRequestSize'] = self.stats['cumulativeRequestSize']
            self.statsAtOnLoadEvent['requestServedFromCacheCount'] = self.stats['requestServedFromCacheCount']
            self.statsAtOnLoadEvent['numObjsLoadedByStatus'] = self.numObjsLoadedByStatus
            self.statsAtOnLoadEvent['responseReceivedCount'] = self.stats['responseReceivedCount']
            self.statsAtOnLoadEvent['numObjsLoadedByType'] = self.numObjsLoadedByType
            self.statsAtOnLoadEvent['numDataChunksReceived'] = self.stats['numDataChunksReceived']
            self.statsAtOnLoadEvent['cumulativeDataLength'] = self.stats['cumulativeDataLength']
            self.statsAtOnLoadEvent['cumulativeEncodedDataLength'] = self.stats['cumulativeEncodedDataLength']
            self.statsAtOnLoadEvent['loadingFinishedCount'] = self.stats['loadingFinishedCount'] # BEWARE these events seem to get logged well after timestamp
            self.statsAtOnLoadEvent['cumulativeEncodedDataLength_LF'] = self.stats['cumulativeEncodedDataLength_LF']
            self.statsAtOnLoadEvent['loadingFailedCount'] = self.stats['loadingFailedCount']
            self.statsAtOnLoadEvent['numBlockedExplicitly'] = self.stats['numBlockedExplicitly']
        elif method == "Page.frameAttached":
            self.stats['frameAttachedCount'] += 1
        elif method == "Page.frameNavigated":
            self.stats['frameNavigatedCount'] += 1
        elif method == "Page.frameDetached":
            self.stats['frameDetachedCount'] += 1
        elif method == "Page.frameStartedLoading":
            self.stats['frameStartedLoadingCount'] += 1
        elif method == "Page.frameStoppedLoading":
            self.stats['frameStoppedLoadingCount'] += 1
        elif method == "Page.frameScheduledNavigation":
            self.stats['frameScheduledNavigationCount'] += 1
        elif method == "Page.frameClearedScheduledNavigation":
            self.stats['frameClearedScheduledNavigationCount'] += 1
        elif method == "Page.frameResized":
            self.stats['frameResizedCount'] += 1
        elif method == "Page.javascriptDialogOpening":
            self.stats['javascriptDialogOpeningCount'] += 1
        elif method == "Page.javascriptDialogClosed":
            self.stats['javascriptDialogClosedCount'] += 1
        elif method == "Page.screencastFrame":
            self.stats['screencastFrameCount'] += 1
        elif method == "Page.screencastVisibilityChanged":
            self.stats['screencastVisibilityChangedCount'] += 1
        elif method == "Page.colorPicked":
            self.stats['colorPickedCount'] += 1
        elif method == "Page.interstitialShown":
            self.stats['interstitialShownCount'] += 1
        elif method == "Page.interstitialHidden":
            self.stats['interstitialHiddenCount'] += 1
        else:
            self.stats['otherPageEventCount'] += 1
            self.logMsgs.append("Other Page event: "+self.rawData_fname+" method: "+method)

    def resetAttributes(self):
        self.msg_list                               = []
        self.frames                                 = {}
        self.firstFrame                             = False
        self.domContentEventFired                   = False
        self.onLoadFired                            = False
        self.dom_timestamp                          = None
        self.onload_timestamp                       = None
        for key in self.timings:
            self.timings[key] = None

        self.resetStatsDict()
        self.makeEmptyAttrDicts()

    def handleRespNetwork(self, resp, method):
        self.stats['NetworkEventCount'] += 1
        if method == "Network.requestWillBeSent":
            self.stats['numObjsRequested'] += 1
            requestSize = len(resp['params']['request'])
            self.stats['cumulativeRequestSize'] += requestSize
            initiator = resp['params']['initiator']
        elif method == "Network.requestServedFromCache":
            self.stats['requestServedFromCacheCount'] += 1
            try:
                self.numObjsLoadedByStatus['cache'] += 1
            except KeyError:
                self.numObjsLoadedByStatus['cache'] = 1
        elif method == "Network.responseReceived":
            self.stats['responseReceivedCount'] += 1
            resp_rcvd_resp = resp['params']['response']
            status = resp_rcvd_resp['status']
            try:
                self.numObjsLoadedByStatus[status] += 1
            except KeyError:
                self.numObjsLoadedByStatus[status] = 1
            resourceType = resp['params']['type']
            try:
                self.numObjsLoadedByType[resourceType] += 1
            except KeyError:
                self.numObjsLoadedByType[resourceType] = 1
        elif method == "Network.dataReceived":
            self.stats['numDataChunksReceived'] += 1
            dataLength = resp['params']['dataLength']
            encodedDataLength = resp['params']['encodedDataLength']
            self.stats['cumulativeDataLength'] += dataLength
            self.stats['cumulativeEncodedDataLength'] += encodedDataLength
        elif method == "Network.loadingFinished":
            self.stats['loadingFinishedCount'] += 1 # BEWARE these events seem to get logged well after timestamp
            encodedDataLength = resp['params']['encodedDataLength']
            self.stats['cumulativeEncodedDataLength_LF'] += encodedDataLength
        elif method == "Network.loadingFailed":
            self.stats['loadingFailedCount'] += 1
            if self.isExplicitlyBlocked(resp):
                self.stats['numBlockedExplicitly'] += 1
        else:
            self.stats['otherNetworkEventCount'] += 1
            self.logMsgs.append("Other Network event: "+self.rawData_fname+" method: "+method)

    def isExplicitlyBlocked(self, resp):
        blocked_msgs = ["net::ERR_BLOCKED_BY_CLIENT", "net::ERR_CONNECTION_REFUSED"]
        errorText = resp['params']['errorText']
        if errorText in blocked_msgs:
            return True
        else:
            return False

    def handleRespTimeline(self, resp, method):
        return

    def handleRespMethod(self, resp, method):
        if method[0:4] == "Page":
            self.handleRespPage(resp, method)
        elif method[0:7] == "Network":
            self.handleRespNetwork(resp, method)
        elif method[0:8] == "Timeline":
            self.handleRespTimeline(resp, method)
        return

    def handleRespNoneMethod(self, resp):
        return


    def _getDataUntilCutoff(self):
        """
        Appends data to the object's msg_list until it finds a message whose
        timestamp is past the cutoff_time.  Saves this last timestamp in the
        object's last_timestamp attribute.
        """
        #this_resp = "{'None': None}"
        while True:
            this_resp = self._getNextWsResp(True)   # returns json obj
            method = self.processResp(this_resp)
            if method == "Page.interstitialShown":
                self.handleInvalidCert(this_resp)
                return

            try:
                self.timings['last_timestamp'] = this_resp["params"]["timestamp"]
            except KeyError as e:
                pass # just check the next timestamp
            else:
                # if we have a value for this_timestamp,
                # use it as first_timestamp if appropriate
                if self.timings['first_timestamp'] == None:
                    self.timings['first_timestamp'] = self.timings['last_timestamp']
                    print("first_timestamp: "+str(self.timings['first_timestamp']))

                # check whether cutoff time has passed
                if self._isPastCutOffTime():
                    self._timeout()
                    print(self.dictToNiceString(this_resp))
                    return

            # if the page simply finishes loading
            if self._finishedLoading():
                print("Finished loading page.  last_timestamp: "+str(self.timings['last_timestamp']))
                return

    def dictToNiceString(self, the_dict):
        nice_str = ""
        if type(the_dict) == type({}):
            for key in the_dict:
                nice_str += ("\n  " + key + ": " + self.dictToNiceString(the_dict[key]))
        else:
            # base case
            nice_str += str(the_dict)[:60]

        return nice_str

    def multithreadingCode():
        '''
        Code that was previously being used for multithreading websocket.recv() timeouts
        '''
        # parent_conn, child_conn = multiprocessing.Pipe()    # open a channel
        # self.p = multiprocessing.Process(target=_getNextWsResp(self.ws, child_conn))
        # self.p.start()
        # self.p.join(3)  # block parent process until timeout or p terminates
        # if self.p.is_alive():
        #     # nothing received through websocket
        #     self.p.terminate()
        #     self.p.join()
        #     parent_conn.close()
        #     child_conn.close()
        #     self._finishedLoading(this_resp, this_timestamp)
        #     return
        #     # this_timestamp = time.time()
        #     # if self._isPastCutOffTime(this_timestamp):
        #     #     self._timeout(this_timestamp)
        #     #     return
        #     # else:
        #     #     # loop again
        #     #     continue
        # else:
        #     # there should be a resp waiting for us on the channel
        #     this_resp = parent_conn.recv()
        #     #print(this_resp)
        # parent_conn.close()
        # child_conn.close()
        return

    def _getHARresp(self):
        while True:
            this_resp = self._getNextWsResp(True)
            this_timestamp = time.time()

            if (this_timestamp - self.timings['first_timestamp']) > self.cutoff_time:
                print("last_timestamp: " + str(this_timestamp))
                self.timings['last_timestamp'] = this_timestamp
                break

    def LoadPage_SaveData(self, dst_url, sample_num):
        """
        Tells the connected browser to load the desired page.  Loads the page for
        the amount of time specified by the object's cutoff_time attribute.  Saves
        data on the page load to the object's msg_list.  When cutoff_time is reached,
        all the data in msg_list is written to a file in the specified output_dir.
        """
        
        loadPageAndSaveDataStart = time.time()

        # update attributes
        self.sample_num = sample_num
        self.page_url = dst_url
        self.hostname = urlparse(self.page_url).netloc

        if self.delay_NLC != None or self.bandwidth_NLC != None:
            network_str = self.network_type+"_"+str(int(self.delay_NLC))+"ms_"+str(self.bandwidth_NLC)+"Kbps"
        else:
            network_str = self.network_type

        self.rawData_fname = self.location+"-"+self.device_name+"-"+network_str+"-"+str(self._isBlockingAds)+"-"+\
                str(sample_num)+"-"+self.hostname+".txt"
        self.summary_fname = self.rawData_fname[:-4]+"-summary.json"

        # Tell browser to load the desired page
        resp = self.navToURL(self.page_url)
        resp_method = self.getRespMethod(resp)
        #print("resp_method: "+resp_method)

        # if the certificate is invalid, it immediately(?) gives Page.interstitialShown response
        if resp_method == "Page.interstitialShown":
            #print("resp is interstitialShown")
            self.handleInvalidCert(resp)
        else:
            # get data and put in msg_list
            #print("moving on to _getDataUntilCutoff")
            self._getDataUntilCutoff()

        # save data to file
        self.writeRawDataToFile()
        self.writeDataSummaryToFile()

        # reset attributes for next page load
        self.resetAttributes()

        loadPageAndSaveDataEnd = time.time()
        self.loadPageAndSaveDataTime += (loadPageAndSaveDataEnd - loadPageAndSaveDataStart)

    def handleInvalidCert(self, resp_obj):
        msg = "Possible invalid certificate encountered. Skipping page: "+self.page_url+" "+self.rawData_fname+" "+str(resp_obj)
        self.logMsgs.append(msg)
        # Note, already being optionally logged to msg_list in _getNextWsResp(LOGRESPONSES)
        print(msg)

    def dumpDataToJSON(self):
        output_file_path = os.path.join(self.output_dir, self.rawData_fname[:-4]+".json")
        with open(output_file_path, "w") as f:
            json.dump(self.msg_list, f)

    def writeRawDataToFile(self):
        print("Writing raw data to file: "+self.rawData_fname)
        output_file_path = os.path.join(self.output_dir, "raw", self.rawData_fname)
        with open(output_file_path, "w") as f:
            for this_item in self.msg_list:
                f.write(json.dumps(this_item)+"\n")

    def setupWebsocket(self):
        print 'in websockets'
        ws_start = time.time()
        msg = "Opening websocket."
        self.msg_list.append(msg)
        # print(msg)
        print msg
        try:
            self.ws = websocket.create_connection(self.url_ws)
        except Exception as e:
            ws_end = time.time()
            self.wsOverhead += (ws_end - ws_start)
            # print(str(e))
            # print("Failed to connect to websocket.")
            print str(e)
            print "Failed to connect to websocket."
            resp_json, shouldQuit = getTabsJSON(self.debug_port)
            tab_number = getTabNumber(resp_json)
            # print("You are driving this tab remotely:")
            # print(resp_json[tab_number])
            print "You are driving this tab remotely:"
            print resp_json[tab_number]
            target_tab = resp_json[tab_number]
            self.url_ws = str(target_tab['webSocketDebuggerUrl'])
            return False

        #mpl.sendMethod("Console.enable", None, True)
        #mpl.sendMethod("Debugger.enable", None, False)
        self.sendMethod("Network.enable", None, True)
        self.sendMethod("Page.enable", None, True)
        #mpl.sendMethod("Runtime.enable", None, True)
        self.sendMethod("Timeline.start", None, True)
        ws_end = time.time()
        self.wsOverhead += (ws_end - ws_start)
        print "WS: ", self.ws
        
        return True

    def runMeasurements(self, this_url, sample_num):
        didSucceed = False
        while didSucceed == False:
            didSucceed = self.setupWebsocket()

        # if useAdBlock:
        #     self.enableAdBlock()
        #     msg = "\nLoading "+this_url+" without ads."
        # else:
        #     self.disableAdBlock()
        #     msg = "\nLoading "+this_url+" with ads."

        #self.clearAllCaches()
        #print(msg)

        #self.closeWebsocket()   # added these two lines to try to correct pop frameLoad error
        #self.setupWebsocket()

        self.LoadPage_SaveData(this_url, sample_num)
        self.clearAllCaches()    ###
        # if useAdBlock:
        #     self.disableAdBlock()   # if we just used the adBlocker, disable it for next run
        # else:
        #     self.enableAdBlock()    # if we just loaded without adBlocker, enable it for next run
        self.closeWebsocket()

    def closeWebsocket(self):
        ws_start = time.time()
        msg = "Closing websocket"
        self.msg_list.append(msg)
        print(msg)
        self.ws.close(timeout=0)  # I belive this was causing my frame pop errors b/c default was to wait 3 seconds for close frame
        self.ws = None
        ws_end = time.time()
        self.wsOverhead += (ws_end - ws_start)

    def setupOutputDirs(self):
        output_dir = self.output_dir
        if not os.path.isdir(output_dir):
            os.mkdir(output_dir)

        log_dir = os.path.join(self.output_dir, "log")
        if not os.path.isdir(log_dir):
            os.mkdir(log_dir)

        summaries_dir = os.path.join(self.output_dir, "summaries")
        if not os.path.isdir(summaries_dir):
            os.mkdir(summaries_dir)

        raw_dir = os.path.join(self.output_dir, "raw")
        if not os.path.isdir(raw_dir):
            os.mkdir(raw_dir)


    # def getHar(self, i):
    #     '''
    #     Tells the connected browser to load the desired page.  Loads the page for
    #     the amount of time specified by the object's cutoff_time attribute.  Saves the HAR
    #     file using the chrome.devtools.network.getHAR() method.
    #     '''
    #     self.ws.send(json.dumps({'id': i, 'method': 'Page.navigate', 'params': {'url': self.page_url}}))
    #     self.first_timestamp = time.time()
    #     self.ws.send(json.dumps({'id': i, 'method': 'chrome.devtools.network.getHAR', 'params': {}}))
    #     self._getHARresp()

    #     # save data to file
    #     self.hostname = urlparse(self.page_url).netloc + "-HAR.txt"
    #     output_file = os.path.join(self.output_dir, self.hostname)
    #     with open(output_file, "w") as f:
    #         json.dump(self.msg_list, f)
