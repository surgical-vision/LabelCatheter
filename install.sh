#!/bin/bash          

# Stop whenever errors happen
set -e

# Current PID
PID=$!

# Current directory
dir=$PWD

# Temporary working directory
label_catheter_tmp_dir="/tmp/LabelCatheter.$RANDOM"
mkdir $label_catheter_tmp_dir
cd $label_catheter_tmp_dir

function clean_up {
	# Perform program exit housekeeping
	# Optionally accepts an exit status
	echo $1
	rm -rf $label_catheter_tmp_dir
	exit 1
}

trap clean_up SIGHUP SIGINT SIGTERM EXIT

# ========== Basic tool installation ===================
printf '\e[1;31m==== Check basic tools ====\e[0;39m\n'

# Check if Homebrew is installed

if ! true; then
	# Install Homebrew
	printf '\e[1;36m==== Install Homebrew ====\e[0;39m\n'	
	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
else
	printf '\e[1;36m==== Update Homebrew ====\e[0;39m\n'	
	brew update
fi
    
# Check if Git is installed
which -s git || brew install git

# Check if cmake is installed
which -s cmake || brew install cmake

# ========== Dependent libraries installation ===================
printf '\n\e[1;31m==== Install dependencies ====\e[0;39m\n'

# GLEW
printf '\e[1;36m==== Check GLEW ====\e[0;39m\n'
glew=$(brew list --versions glew)
if [[ -n $glew ]]; then
	echo "$glew installed"
else
	brew install glew	
fi

# Boost
printf '\e[1;36m==== Check Boost ====\e[0;39m\n'
boost=$(brew list --versions boost)
if [[ -n $boost ]]; then 
	echo "$boost installed" 
else
	brew install boost
fi

# FFmpeg
printf '\e[1;36m==== Check FFmpeg ====\e[0;39m\n'
ffmpeg=$(brew list --versions ffmpeg)
if [[ -n $ffmpeg ]]; then 
	echo "$ffmpeg installed" 
else
	brew install ffmpeg
fi

# Eigen
printf '\e[1;36m==== Check Eigen ====\e[0;39m\n'
eigen=$(brew list --versions eigen)
if [[ -n $eigen ]]; then 
	echo "$eigen installed" 
else
	brew install eigen
fi

# Pangolin
printf '\e[1;36m==== Check Pangolin ====\e[0;39m\n'
if [ ! -f /usr/local/lib/libpangolin* ]; then
	printf '\e[1;36m==== Build Pangolin ====\e[0;39m\n'
	git clone https://github.com/stevenlovegrove/Pangolin.git Pangolin
	cd Pangolin; mkdir build; cd build

	cmake .. -DEXPORT_Pangolin=OFF -DBUILD_EXAMPLES=OFF || clean_up "cmake failed" 
	make -j8 || clean_up "make failed"
	make install
else
	echo "Pangolin installed"
fi

# ========== Build LabelCatheter ===================
printf '\n\e[1;31m==== Building LabelCatheter ====\e[0;39m\n'
git clone https://github.com/surgical-vision/LabelCatheter.git LabelCatheter
cd LabelCatheter
mkdir build; cd build
cmake .. || clean_up "cmake failed"
make -j8 || clean_up "make failed"

# ========== Installation ===================
printf '\n\e[1;31m==== Installing /usr/local/bin/{label_catheter, video_exporter} ====\e[0;39m\n'
cp label_catheter /usr/local/bin/label_catheter
cp video_exporter /usr/local/bin/video_exporter

# ========== Cleanup ===================
cd $dir
rm -rf $label_catheter_tmp_dir 

echo "Done."
