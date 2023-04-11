#!/usr/bin/env bash

#####
# THIS FILE IS USED TO GENERATE "SELF-CONTAINMENT" CHECKS
#
# make ABSOLUTELY sure that your CWD is the directory that contains this file
# before running this script. i'm not liable for any demolished directories...
#
# don't set `TRACER_CHECK_SELF_CONTAINMENT` to ON in CMakeLists.txt before
# running this script.
#
# "self-containment checks" is a term i have just pulled out of my ass to
# describe the process of checking all .hpp files to see if their includes
# are "sufficient" for the classes and functions they refer to.
#
# it is *NORMAL* to get a linker error after compiling the target
# `tracer_sc_checks`, the main function is not defined. as long as the thing
# compiles, it means that the "check" has passed.
#
####

[[ -d sc_checks ]] && (rm -ri sc_checks || exit 1)
[[ -d sc_checks ]] || (mkdir sc_checks || exit 2)

#sc_wd="$(pwd)/sc_checks"

cd ../include/ || exit 1

for f in $(find -type f -name "*.hpp"); do
  target_file="../src/sc_checks/${f}.cpp"
  target_dirname=$(dirname "${target_file}")

  [[ -d "${target_dirname}" ]] || mkdir -p "${target_dirname}"
  #echo "$(pwd)/${target_file}"
  printf "#include \"$(pwd)/../include/%s\"\n\n" "$f" > "${target_file}"
done
