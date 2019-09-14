# Scooter Board MCU

This project is a base project for my own idea of writing a universal code for controllers that could control e-scooters and possibly replace the ones that already exist. 

Code here is written keeping in mind that any controller could use it. For now it is tested on Arduino MEGA2560.

# PlatformIO

Whole project is based on PlatformIO environment, using VSCode and its plugin. It is recommended to use same setup.

# Testing

All code that is going to be a shared library is aiming to have test cases for it. For that purposes two libraries are used:

[FakeIT](https://github.com/eranpeer/FakeIt/wiki/Quickstart)
[unity](https://github.com/ThrowTheSwitch/Unity)

## Running tests

To run tests, run following command

```
platformio test -v -e native-test
```