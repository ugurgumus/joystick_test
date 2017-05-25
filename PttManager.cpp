
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/hidraw.h>
#include <linux/input.h>

#include <libudev.h>

#include "PttManager.h"

std::string PttManager::m_PttDevice;

PttManager::PttManager()
: m_IsRunning(false),
  m_FD(-1)
{
}

PttManager::PttManager(std::string const& deviceNodePath)
: m_DeviceNodePath(deviceNodePath),
  m_IsRunning(true),
  m_FD(-1)
{
    m_PttEvent.SourceId = 1;
    m_ThreadInstance.reset(new boost::thread(boost::bind(&PttManager::runThread, this)));
}

PttManager::~PttManager()
{
    printf("PttManager is being destroyed...\n");

    m_IsRunning = false;

    if (!m_ThreadInstance->try_join_for(boost::chrono::milliseconds(5000)))
    {
        m_ThreadInstance->interrupt();
    }
}

PttManager* PttManager::initialize(std::string const& pttDevice)
{
    PttManager* pttManager = 0;

    m_PttDevice = pttDevice;

    std::string deviceNodePath;
    if (findPttDev(pttDevice, deviceNodePath))
    {
        pttManager = new PttManager(deviceNodePath);
    }

    return pttManager;
}

bool PttManager::findPttDev(std::string const& pttDevice, std::string& deviceNodePath)
{
    bool found = false;
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    std::vector<std::string> devAttrVec;
    std::vector<std::string>::const_iterator devAttrIt;
    devAttrVec.push_back("idVendor");
    devAttrVec.push_back("idProduct");
    devAttrVec.push_back("manufacturer");
    devAttrVec.push_back("product");
    devAttrVec.push_back("serial");

    // Create the udev object
    udev = udev_new();
    if (!udev)
    {
        printf( "Error: Can't create udev!!!\n");
        return false;
    }

    // Create a list of the devices in the 'hidraw' subsystem.
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    // For each item enumerated, print out its information.
    // udev_list_entry_foreach is a macro which expands to
    // a loop. The loop will be executed for each member in
    // devices, setting dev_list_entry to a list entry
    // which contains the device's path in /sys.
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char *path;

        /// Get the filename of the /sys entry for the device
        //  and create a udev_device object (dev) representing it
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        // usb_device_get_devnode() returns the path to the device node itself in /dev.
        deviceNodePath = udev_device_get_devnode(dev);

        // The device pointed to by dev contains information about
        // the hidraw device. In order to get information about the
        // USB device, get the parent device with the
        // subsystem/devtype pair of "usb"/"usb_device". This will
        // be several levels up the tree, but the function will find it.
        dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!dev)
        {
            printf( "Error: Unable to find parent USB device!!!\n");
            return false;
        }

        // From here, we can call get_sysattr_value() for each file
        // in the device's /sys entry. The strings passed into these
        // functions (idProduct, idVendor, serial, etc.) correspond
        // directly to the files in the directory which represents
        // the USB device. Note that USB strings are Unicode, UCS2
        // encoded, but the strings returned from
        // udev_device_get_sysattr_value() are UTF-8 encoded.
        for (devAttrIt = devAttrVec.begin(); devAttrIt != devAttrVec.end(); ++devAttrIt)
        {
            const char* deviceInfo = udev_device_get_sysattr_value(dev, devAttrIt->c_str());

            if (deviceInfo) {

                printf("%s: %s\n", devAttrIt->c_str(), deviceInfo);

                found = (strstr(deviceInfo, pttDevice.c_str()) != 0);
                if (found) {
                    break;
                }
            }
        }

        udev_device_unref(dev);

        if (found) {
            break;
        }
    }
    // Free the enumerator object
    udev_enumerate_unref(enumerate);

    udev_unref(udev);

    return found;
}

static const char* bus_str(int bus)
{
    switch (bus)
    {
    case BUS_USB:
        return "USB";
        break;
    case BUS_HIL:
        return "HIL";
        break;
    case BUS_BLUETOOTH:
        return "Bluetooth";
        break;
    case BUS_VIRTUAL:
        return "Virtual";
        break;
    default:
        return "Other";
        break;
    }
}

void PttManager::printDevInfo(int fd)
{
    char buf[256];
    struct hidraw_devinfo info;

    ::memset(&info, 0x0, sizeof(info));
    ::memset(buf, 0x0, sizeof(buf));

    int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
    if (res < 0) {
        perror("HIDIOCGRAWNAME");
    }
    else {
        printf("Raw Name: %s\n", buf);
    }

    // Get Physical Location
    res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);

    if (res < 0) {
        perror("HIDIOCGRAWPHYS");
    }
    else {
        printf("Raw Phys: %s\n", buf);
    }

    // Get Raw Info
    res = ioctl(fd, HIDIOCGRAWINFO, &info);

    if (res < 0) {
        perror("HIDIOCGRAWINFO");
    }
    else {
        printf("Raw Info:\n");
        printf("\tbustype: %d (%s)\n", info.bustype, bus_str(info.bustype));
        printf("\tvendor: 0x%04hx\n",  info.vendor);
        printf("\tproduct: 0x%04hx\n", info.product);
    }
}

bool PttManager::openDevice(char const* const dev)
{
    // Open Blocked
    m_FD = ::open(dev, O_RDWR);

    if (m_FD < 0) {
        std::string errMsg = "Error: Unable to open device " + std::string(dev);
        perror(errMsg.c_str());
        return false;
    }
    else {
            printf("PTT Device Node: %s\n", dev);
    }

    return true;
}

bool PttManager::tryToReOpenDevice()
{
    printf( "Trying re-open device %s\n", m_PttDevice.c_str());

    if (findPttDev(m_PttDevice, m_DeviceNodePath))
    {
        return openDevice(m_DeviceNodePath.c_str());
    }

    return false;
}

void PttManager::runThread()
{
    // USB Ptt Events
    static const unsigned char PTT_KEY_RELEASED          = 0x00;
    static const unsigned char PTT_KEY_IS_BEING_PRESSED  = 0x01;

    // Joystick Ptt Events
    static const unsigned char JS_KEY_RELEASED          = 0x7F;
    static const unsigned char JS_KEY_IS_BEING_PRESSED  = 0xFF;

    enum ePttState
    {
        ePTT_RELEASED,
        ePTT_PRESSED
    };

    ePttState state = ePTT_RELEASED;
    bool stateDev4 = false;
    bool stateDev1 = false;

    try
    {
        if (!m_DeviceNodePath.empty() && m_IsRunning)
        {
            if (!openDevice(m_DeviceNodePath.c_str()))
            {
                return;
            }

            printDevInfo(m_FD);

            unsigned char eventBuffer[16];
            const int read_size=2;
            int res = 0;
            while (m_IsRunning)
            {
                res = read(m_FD, eventBuffer, read_size);

                usleep(500000);
                if (res < 0)
                {
                    perror("PttManager Error");
                    ::close(m_FD);

                    if (!tryToReOpenDevice())
                    {
                        ::sleep(5);
                    }
                }
                else {

                    if (res >= 2)
                    {
                        for (int i = 0 ; i < read_size; i++) {
                            printf("%02X ",  eventBuffer[i]);
                        }
                        puts("");

                        // Handle USB Ptt Events
                        if (eventBuffer[1] == 0x00)
                        {
                            switch (eventBuffer[0])
                            {
                            case PTT_KEY_IS_BEING_PRESSED:
                                if (state != ePTT_PRESSED)
                                {
                                    state = ePTT_PRESSED;
                                    {
                                        printf( "Ptt Key Pressed\n");
                                    }
                                }
                                break;
                            case PTT_KEY_RELEASED:
                            {
                                if (state != ePTT_RELEASED)
                                {
                                    state = ePTT_RELEASED;

                                    {
                                        printf( "Ptt Key Released\n");
                                    }

                                    PttAdapter::pttReleased(m_PttEvent);
                                }
                            }
                                break;
                            default:
                                state = ePTT_RELEASED;
                                puts( "Invalid Ptt event\n");
                                break;
                            }
                        }
                        // Handle joystick Ptt Events
                        else if (eventBuffer[0] == JS_KEY_IS_BEING_PRESSED || eventBuffer[0] == JS_KEY_RELEASED)
                        {
                            if (eventBuffer[0] == JS_KEY_IS_BEING_PRESSED)
                            {
                                // handset ptt pressed
                                if (stateDev4 == false)
                                {
                                    stateDev4 = true;

                                    m_PttEvent.SourceId = 4;
                                    PttAdapter::pttPressed(m_PttEvent);
                                }
                            }
                            else if (eventBuffer[0] == JS_KEY_RELEASED)
                            {
                                // handset ptt released
                                if (stateDev4 == true)
                                {
                                    stateDev4 = false;
                                    m_PttEvent.SourceId = 4;
                                    PttAdapter::pttReleased(m_PttEvent);
                                }
                            }

                            
                            if (eventBuffer[1] == JS_KEY_IS_BEING_PRESSED)
                            {
                                // headset ptt pressed
                                if (stateDev1 == false)
                                {
                                    stateDev1 = true;
                                    m_PttEvent.SourceId = 1;
                                    PttAdapter::pttPressed(m_PttEvent);
                                }
                            }
                            else if (eventBuffer[1] == JS_KEY_RELEASED)
                            {
                                // headset ptt released
                                if (stateDev1 == true)
                                {
                                    stateDev1 = false;
                                    m_PttEvent.SourceId = 1;
                                    PttAdapter::pttReleased(m_PttEvent);
                                }
                            }
                        }
                    }
                }
            }

            ::close(m_FD);
        }
    }
    catch (boost::thread_interrupted const&)
    {
        printf( "Ptt Thread interrupted\n");
    }
}
