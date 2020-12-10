# MBZ

## AUTHORS

James Newman, Abbas Razaghpanah, Narseo Vallina-Rodriguez, Fabián E. Bustamante, Mark Allman, Diego Perino, Alessandro Finamore

## CONTENTS

mbz: the main MBZ Android Studio project.

pcpp: a snapshot of the original pcap++ code. MBZ (currently) uses an
unmodified copy of this located in the mbz directory.

plugins: contains a set of Android Studio projects, one for each plugin.

test: a (very) simple test client and server, meant for unit tests. Could
do more interesting tests with this. Requires a cross-compiling toolchain
for the client. Currently implements an echo test only.

## DEPLOYMENT

The MBZ project can be deployed through Android Studio directly.

Plugins need to be deployed manually to the device. The following files need
to exist in the device's directory 
/data/data/<mbz-package-name>/files/plugins/<plugin-name>:
  - config (plain text)
  - <plugin-name>.jar (Android APK for UI code)
  - lib<plugin-name>.so (C++ shared library for native code)

For example, the 'snitch' plugin, when installed under an MBZ application
named 'com.nalindquist.mbz', will have the following files in
/data/data/com.nalindquist.mbz/files/plugins/snitch:
  - config
  - snitch.jar
  - libsnitch.so

The config file is a set of lines, with each line containing one integer. It's
structured like this:
  is-pipeline
  service-id-0
  ...
  service-id-n
Use 1 for is-pipeline to insert the plugin into the pipeline. Then follow with
zero or more service IDs, which specify which services (APIs) the plugin 
depends on.

Currently defined services are:
  0: flow-stats (info about active connections)
  1: ui (pass data to the UI)
  2: wifi (control the WiFi interface)

Build the plugin project in Android studio. I've been using 
plugins/<plugin-name>/app/build/outputs/apk/debug/app-debug.apk
for the .jar file, and
plugins/<plugin-name>/app/build/intermediates/cmake/debug/obj/<isa>/libplugin.so
for the .so file.

## IMPLEMENTATION - MANAGED

The Java code has just a few important classes:
  MbzActivity: main activity
  MbzService: main service (foreground)
  Plugin: knows about the plugin filesystem structure and loads code from the
          .jar file
  PluginActivity: used to display plugin UIs
  RouterController: the (only) interface to the MBZ native router

Note that PluginController is pretty much vestigial at this point.

## IMPLEMENTATION - NATIVE

The code is split into four namespaces:
  common: general utilities
  pcpp: the Pcap++ code, unmodified
  routing: the main router, flows, TCP state, and JNI
  services: implementations of APIs for plugins (e.g. wifi, flow-stats)

mbz-lib.cpp, outside these namespaces, has all entry points from managed code.

The threading model might not be obvious. Managed code (RouterController)
spawns a thread that executes Router::run(). From the point of view of the
native code, this is the main thread. (And the only thread that is allowed to
use JNI; JNI gets weird with multiple threads.)

The main thread handles high-priority work, and must never block (except when
select() determines there is no work). When a flow is created, it runs on
its own thread. Flow threads must not block. When a plugin is created, it also
gets its own thread. Any services that are created to support the plugin get
their own thread. Plugin and service threads may (and usually do) block. Note
that by block, I mean block on a request to another software component 
(router, flow, plugin, service), not necessarily on a system call. These rules
help avoid deadlock, though deadlock may still be possible.

My model for concurrency control is basically message passing. For example,
from the main thread, the router might submit an outgoing packet to a flow. 
Though this is implemented as a function call, it's really enqueueing a
message that will be processed asynchronously by the flow thread. Also, the
router is the only software component that has "permission" to send messages
to everyone. All other software components (flow, plugin, service) send and
receive messages only from the router. I chose message passing to limit
concurrency problems (e.g., the router's private data structures don't have
to be guarded by locks), but it probably causes a performance hit.

The only place where message passing isn't used is in calls from the router
into a plugin. This is to keep plugins as general as possible, but plugins
have caused me more concurrency problems.

Some miscellaneous notes:

Router.cpp is the main class. Only code in or called from run() is executed on 
the main thread.

Jni.cpp is the only class allowed to call JNI functions, and is only executed
on the main thread.

InPacket.cpp and OutPacket.cpp are wrappers for pcpp.

Utils.cpp has debug prints, which are controlled by preprocessor symbols
defined in the cmake build file.

## IMPLEMENTATION - DIRTY LAUNDRY

There's a few bad things going on.

Message passing uses concurrent producer-consumer queues. Right now, those
queues are Unix pipes. This is probably really, really bad (but it did make
the implementation simple).

Certain things are hard-coded, e.g. the address of the tun interface and the
MTU.

There is a lot of dynamic memory allocation happening. So there's probably a
lot of memory leaks, too.
