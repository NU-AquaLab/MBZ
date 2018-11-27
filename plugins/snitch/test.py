import requests
import time
import json
import argparse
from MeasurePageLoad import MeasurePageLoad, connectToDevice, getNetworkType, shouldContinue, getDelayAndBandwidth
from MeasurePageLoad import fixURL, getLocation, attemptConnection, getUserOS, getScreenDimensions
from MeasurePageLoad import getTabNumber, getTabsJSON, getMeasureTarget

def parse_args():
    parser = argparse.ArgumentParser(
            description='load some pages and collect some data')
    parser.add_argument('cutoff_time', type=int,
                    help="The number of seconds to allow a page to load "
                        "(while collecting data) before loading the next page.")
    parser.add_argument('num_samples', type=int,
                    help="The number of times to load each page with and without ads.")
    parser.add_argument('url_list_file', type=str,
                    help="A file containing a list or dict of urls to load.  Each page "
                        "is loaded for the amount of time specified by the cutoff_time argument.")
    parser.add_argument('output_dir', type=str,
                    help="A directory where the output data files will be written.")
    parser.add_argument('debug_port', type=int,
                    help="The port to use as the 'remote-debugging-port'")
    return parser.parse_args()

if __name__ == "__main__":
    # Get arguments
    args = parse_args()
    cutoff_time = args.cutoff_time
    url_list_file = args.url_list_file
    output_dir = args.output_dir
    debug_port = args.debug_port
    num_samples = args.num_samples

    # get list of URLs from file
    with open(url_list_file, 'r') as f:
            url_list = json.load(f)

    num_urls = len(url_list)
    min_time = num_urls * 2 * num_samples * cutoff_time/60.0
    print("\nThis will require at least "+str(min_time)+" minutes to complete.")
    if not shouldContinue():
        print("Canceled.")
        exit()
    
    # connect to device
    device_name, device_type, op_sys = connectToDevice(debug_port)

    # get location
    location = getLocation()

    # get network connection type
    network_type = getNetworkType(device_type)
    delay_NLC, bandwidth_NLC = getDelayAndBandwidth(device_type)

    # get screen dimensions
    if device_type == "phone":
        screen_width, screen_height = getScreenDimensions()
    else:
        screen_width = None
        screen_height = None

    if device_type == "computer":
        measure = getMeasureTarget()
    else:
        measure = "ads"
    
    # Connect to browser and gather json info
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
    
    mpl = MeasurePageLoad(url_ws, cutoff_time=cutoff_time, device_name=device_name, device_type=device_type, delay_NLC=delay_NLC,
                        bandwidth_NLC=bandwidth_NLC, debug_port=debug_port, network_type=network_type, location=location, output_dir=output_dir, op_sys=op_sys,
                        start_time=start_time, min_time=min_time, screen_width=screen_width, screen_height=screen_height, measure=measure)

    mpl.setupOutputDirs()

    # Do this once before we start
    # Every other time it happens at end of runMeasurements
    mpl.setupWebsocket()
    mpl.clearAllCaches()
    mpl.enableAdBlock()     # first run is with ad-blocker (without ads)
    mpl.closeWebsocket()

    for this_url in url_list:
        try:
            mpl.categories_and_ranks = url_list[this_url]['category_ranks']
        except TypeError:
            pass

        this_url = fixURL(this_url)
        
        for sample_num in range(1, num_samples+1):
            # run with ad-blocker ON
            useAdBlocker = True
            msg = "\nLoading "+this_url+" without ads.  sample_num: "+str(sample_num)
            print(msg)
            mpl.runMeasurements(useAdBlocker, this_url, sample_num)

            # run with ad-blocker OFF
            useAdBlocker = False
            msg = "\nLoading "+this_url+" with ads.  sample_num: "+str(sample_num)
            print(msg)
            mpl.runMeasurements(useAdBlocker, this_url, sample_num)
        # end looping through sample_nums
    # end looping through all urls


    mpl.writeLog()

    print("TODO: add outgoing and incoming data")

