from MeasurePageLoad import MeasurePageLoad, connectToDevice, getNetworkType, shouldContinue, getDelayAndBandwidth 
from MeasurePageLoad import fixURL, getLocation, attemptConnection, getUserOS, getScreenDimensions
from MeasurePageLoad import getTabNumber, getTabsJSON, getMeasureTarget
import time

url_list = ['www.nytimes.com', 
        'www.bbc.com', 
        'www.ft.com', 
        'www.msn.com', 
        'www.github.com', 
        'www.reddit.com', 
        'www.yahoo.com', 
        'www.washingtonpost.com', 
        'www.latimes.com', 
        'www.cnn.com', 
        'www.theguardian.com', 
        'www.independent.co.uk', 
        'npr.org', 
        'www.cnbc.com', 
        'www.time.com', 
        'www.cbsnews.com', 
        'cbc.ca', 
        'www.abcnews.go.com', 
        'www.vox.com',
        'www.rawstory.com']
debug_port = 9223

# connect to device
device_name, device_type, op_sys = connectToDevice(debug_port)

# get location
location = getLocation()
#
#         # get network connection type
network_type = getNetworkType(device_type)
delay_NLC, bandwidth_NLC = getDelayAndBandwidth(device_type)
if device_type == "phone":
    screen_width, screen_height = getScreenDimensions()
else:
    screen_width = None
    screen_height = None

measure = "third-party"

resp_json, shouldQuit = getTabsJSON(debug_port)
if shouldQuit:
    print("Script aborted.")
    exit()
# Choose a browser tab to drive remotely
if len(resp_json) > 1:
    tab_number = getTabNumber(resp_json)
else:
    tab_number = 0

start_time = time.time()

print("You are driving this tab remotely:")
print(resp_json[tab_number])

target_tab = resp_json[tab_number]   # Possibly need to run adb command to open chrome in the first place?

#websocket.enableTrace(True) # Not exactly sure what this does

# connect to first tab via the WS debug URL
url_ws = str(target_tab['webSocketDebuggerUrl'])
cutoff_time = 120
output_dir = 'output'
min_time = 100
mpl = MeasurePageLoad(url_ws, cutoff_time=cutoff_time, device_name=device_name, device_type=device_type, delay_NLC=delay_NLC,
                    bandwidth_NLC=bandwidth_NLC, debug_port=debug_port, network_type=network_type, location=location, output_dir=output_dir, op_sys=op_sys,
                    start_time=start_time, min_time=min_time, screen_width=screen_width, screen_height=screen_height, measure=measure)

mpl.setupOutputDirs()

# Do this once before we start
# Every other time it happens at end of runMeasurements
mpl.setupWebsocket()
mpl.clearAllCaches()
# mpl.enableAdBlock()     # first run is with ad-blocker (without ads)
mpl.closeWebsocket()

for this_url in url_list:
    try:
        mpl.categories_and_ranks = url_list[this_url]['category_ranks']
    except TypeError:
        pass

    this_url = fixURL(this_url)
    num_samples = 1 
    for sample_num in range(1, num_samples+1):
        mpl.runMeasurements(this_url, sample_num)


