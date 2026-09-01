cmake_minimum_required(VERSION 3.14)
project(GUI_App LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON) # Required for Qt Signals/Slots
set(CMAKE_AUTOUIC ON) # If using .ui files
set(CMAKE_AUTORCC ON)
# Find core base packages first

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    SerialPort 
    Charts
)

# Define your executable target
add_executable(GUI_App
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# Link the Qt6 modules
target_link_libraries(GUI_App PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Charts
)

