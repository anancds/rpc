#!/bin/bash

../../../build/even-http/ps/core/scheduler_example > scheduler.log 2>&1 &
sleep 1

../../../build/even-http/ps/core/server_example > server.log 2>&1 &
sleep 1

../../../build/even-http/ps/core/server_example1 > server1.log 2>&1 &
sleep 1


../../../build/even-http/ps/core/client_example > worker.log 2>&1 &
