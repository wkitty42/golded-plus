set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(gmb3_SOURCES
  gmoarea.cpp
  gmohuds.cpp
  gmoezyc1.cpp
  gmoezyc2.cpp
  gmoezyc3.cpp
  gmoezyc4.cpp
  gmoezyc5.cpp
  gmofido1.cpp
  gmofido2.cpp
  gmofido3.cpp
  gmofido4.cpp
  gmofido5.cpp
  gmojamm1.cpp
  gmojamm2.cpp
  gmojamm3.cpp
  gmojamm4.cpp
  gmojamm5.cpp
  gmopcbd1.cpp
  gmopcbd2.cpp
  gmopcbd3.cpp
  gmopcbd4.cpp
  gmopcbd5.cpp
  gmosmb1.cpp
  gmosmb2.cpp
  gmosqsh1.cpp
  gmosqsh2.cpp
  gmosqsh3.cpp
  gmosqsh4.cpp
  gmosqsh5.cpp
  gmowcat1.cpp
  gmowcat2.cpp
  gmowcat3.cpp
  gmowcat4.cpp
  gmowcat5.cpp
  gmoxbbs1.cpp
  gmoxbbs2.cpp
  gmoxbbs3.cpp
  gmoxbbs4.cpp
  gmoxbbs5.cpp
)
include_directories(../gall ../gcfg ../gcui ../smblib ../glibc)
ADD_LIBRARY(gmb3 STATIC ${gmb3_SOURCES})