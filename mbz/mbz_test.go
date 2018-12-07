package main

import (
	"aqualab/adb_helper"
	// "fmt"
	"flag"
	"testing"
)

var test_app = flag.String("test_app", "", "name of activity to start")
var debug_apk = flag.String("debug_apk", "app/build/outputs/apk/debug/app-debug.apk", "location of debug_apk")

// func main() {
// 	fmt.Println("vim-go")
// }

func TestMBZ(t *testing.T) {
	adb_helper.Init(t)
	adb_helper.RunApp(t, *debug_apk, *test_app)
	adb_helper.Wait(1000)
	adb_helper.CloseApp(t)
}
