#!/usr/bin/env bash

ATS=${1:-../ts}

if [ ! -d "${ATS}" ] ; then
  echo "Target ${ATS} is not a directory"
  exit 1
fi

if [ ! -r Sconstruct ] ; then
  echo "This must be run from the top level libswoc directory."
  exit 2
fi

BASE_PATH="lib"
TARGET_BASE_DIR="${ATS}/${BASE_PATH}"

SRC_PATH="${BASE_PATH}/swoc/src"
TARGET_SRC_DIR="${ATS}/${SRC_PATH}"
if [ -d "${TARGET_SRC_DIR}" ] ; then
  rm -f ${TARGET_SRC_DIR}/*.cc
else
  mkdir -p ${TARGET_SRC_DIR}
fi

cp code/src/*.cc ${TARGET_SRC_DIR}
(cd ${ATS}; git add ${BASE_PATH}/swoc/CMakeLists.txt ; git add ${SRC_PATH}/*.cc)

INC_PATH="${BASE_PATH}/swoc/include/swoc"
TARGET_INC_DIR="${ATS}/${INC_PATH}"

if [ -d "${TARGET_INC_DIR}" ] ; then
  rm -f ${TARGET_INC_DIR}/*.h
else
  mkdir -p ${TARGET_INC_DIR}
fi

if [ -d "${TARGET_INC_DIR}/ext" ] ; then
  rm -f ${TARGET_INC_DIR}/ext/*.h
else
  mkdir -p ${TARGET_INC_DIR}/ext
fi

cp code/include/swoc/*.h ${TARGET_INC_DIR}
cp code/include/swoc/ext/*.h ${TARGET_INC_DIR}/ext
cp code/include/swoc/ext/HashFNV.h ${TARGET_INC_DIR}
(cd ${ATS}; git add ${INC_PATH}/*.h ; git add ${INC_PATH}/ext/*.h)
