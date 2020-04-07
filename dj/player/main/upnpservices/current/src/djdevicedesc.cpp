char* gc_szDJDeviceDescXMLBase = 

"<?xml version=\"1.0\"?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion>"
"    <major>1</major>"
"    <minor>0</minor>"
"  </specVersion>"                                                                                      // sprintf targets:
"  <device>"
"    <deviceType>urn:www-fullplaymedia-com:device:darwinJukebox:1</deviceType>"
"    <friendlyName>Fullplay DJ-C40 %s</friendlyName>"
"    <manufacturer>Fullplay Media Systems, Inc.</manufacturer>"
"    <manufacturerURL>http://www.fullplaymedia.com</manufacturerURL>"
"    <modelDescription>Fullplay DJ-C40</modelDescription>"
"    <modelName>DJ-C40</modelName>"
"    <modelNumber>1.0</modelNumber>"
"    <modelURL>http://www.fullplaymedia.com</modelURL>"
"    <serialNumber>01</serialNumber>"
"    <UDN>uuid:FullPlay_DJ_C40_%s</UDN>"                                                                  // udn uniqueifier (part of mac?)
"    <UPC>1</UPC>"
"    <serviceList>"
"      <service>"
"        <serviceType>urn:www-fullplaymedia-com:service:X_FullPlayDeviceServices:1</serviceType>"
"        <serviceId>urn:upnp-org:serviceId:X_FullPlayDeviceServices</serviceId>"
"        <controlURL>http://%s:%d/control/X_FPDS</controlURL>"                                                 // ip, port
"        <eventSubURL>http://%s:%d/event/X_FPDS</eventSubURL>"                                                 // ip, port
"        <SCPDURL>http://%s:%d/SCPD.xml</SCPDURL>"                                                             // ip, port
"      </service>"
/*      <service>"
        <serviceType>urn:schemas-upnp-org:service:tvpicture:1</serviceType>"
        <serviceId>urn:upnp-org:serviceId:tvpicture1</serviceId>"
        <controlURL>/upnp/control/tvpicture1</controlURL>"
        <eventSubURL>/upnp/event/tvpicture1</eventSubURL>"
        <SCPDURL>/tvpictureSCPD.xml</SCPDURL>"
      </service>"
*/
"    </serviceList>"
"   <presentationURL>http://%s/index.htm</presentationURL>" // ENTER DEFAULT URL
"</device>"
"</root>";
