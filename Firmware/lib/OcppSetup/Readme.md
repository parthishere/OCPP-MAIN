How to use it??

Include OcppSetup library to ArduinoOcpp.h

```
#include <OcppSetup.h>
```

Then add this line to ArduinoOcpp.cpp in the namespace "ArduinoOcpp"
OcppSetup ocppsetup;

So your namespace might look like this

```
namespace ArduinoOcpp
{
    namespace Facade
    {
    ......
    } // end namespace ArduinoOcpp::Facade
    OcppSetup ocppsetup;
} // end namespace ArduinoOcpp
```

After that you can initialize the lcd in the ArduinoOcpp.cpp file and in the OcppInitialize Function,

Add ArduinoOcpp.cpp file whenever the lcd led or any other function are needed!

Some pre defined function to use :
lcdInitialize(); -> initialze the lcd
