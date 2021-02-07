#!/bin/bash

while [ 1 ]
do
  curl -X POST -d 'postKey=postValue' -i -H "Accept: application/json" -H "headerKey: headerValue"  "http://127.0.0.1:9999/handler?key1=value1"

done