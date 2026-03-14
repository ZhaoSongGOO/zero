#!/bin/bash
current_dir=$(pwd)
script_dir=$(cd $(dirname $0); pwd)
gn_file=$current_dir/.gn
$(rm $gn_file)
