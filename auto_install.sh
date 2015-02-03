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
printf '\n\e[1;31m==== Check basic tools ====\e[0;31m\n'

# Check if Homebrew is installed
which -s brew
if [[ $? != 0 ]]; then
	# Install Homebrew
	printf '\n\e[1;36m==== Install Homebrew ====\e[0;39m\n'	
	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
else
	printf '\n\e[1;36m==== Update Homebrew ====\e[0;39m\n'	
	brew update
fi
    
# Check if Git is installed
which -s git || brew install git

# Check if cmake is installed
which -s cmake || brew install cmake

# ========== Dependent libraries installation ===================
printf '\n\e[1;31m==== Install dependencies ====\e[0;31m\n'

# GLEW
printf '\n\e[1;36m==== Check GLEW ====\e[0;39m\n'
glew=$(brew list --versions glew)
if [[ -n $glew ]]; then
	echo "$glew installed"
else
	brew install glew	
fi

# Boost
printf '\n\e[1;36m==== Check Boost ====\e[0;39m\n'
boost=$(brew list --versions boost)
if [[ -n $boost ]]; then 
	echo "$boost installed" 
else
	brew install boost
fi

# Eigen
printf '\n\e[1;36m==== Check Eigen ====\e[0;39m\n'
eigen=$(brew list --versions eigen)
if [[ -n $eigen ]]; then 
	echo "$eigen installed" 
else
	brew install eigen
fi

# OpenCV
printf '\n\e[1;36m==== Check OpenCV ====\e[0;39m\n'
opencv=$(brew list --versions opencv)
if [[ -n $opencv ]]; then 
	echo "$opencv installed" 
else
	brew tap homebrew/science
	brew install opencv --with-ffmpeg
fi

# Pangolin
printf '\n\e[1;36m==== Check Pangolin ====\e[0;39m\n'
if [ ! -f /usr/local/lib/libpangolin* ]; then
	printf '\n\e[1;36m==== Build Pangolin ====\e[0;39m\n'
	git clone https://github.com/stevenlovegrove/Pangolin.git Pangolin
	cd Pangolin; mkdir build; cd build

	cmake .. -DEXPORT_Pangolin=OFF -DBUILD_PANGOLIN_VIDEO=OFF -DBUILD_EXAMPLES=OFF || clean_up "cmake failed" 
	make -j8 || clean_up "make failed"

	make install
	cd $label_catheter_tmp_dir
	rm -rf Pangolin
else
	echo "Pangolin installed"
fi

# ========== Build LabelCatheter ===================
printf '\n\e[1;36m==== Building LabelCatheter ====\e[0;39m\n'
git clone https://github.com/surgical-vision/LabelCatheter.git LabelCatheter
cd LabelCatheter
mkdir build; cd build
cmake .. || clean_up "cmake failed"
make -j8 || clean_up "make failed"

# ========== Installation ===================
cp LabelCatheter $dir/
cp -r ../osx/LabelCatheter.app $dir

# ========== Cleanup ===================
cd $dir
rm -rf $label_catheter_tmp_dir 
