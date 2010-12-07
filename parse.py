#!/usr/bin/python

# Parse a dump file from SysNucleus USBTrace, and extract information about
# control and bulk transfers, filtering out all the other USB bus operation
# gunk.
#
import libxml2
import sys

# Pad a hex number with leading zeros to make a 16-bit value 0xWXYZ.
# 0x1     -> 0x0001
# 0x12    -> 0x0012
# 0x123   -> 0x0123
# 0x1234  -> 0x1234
# 0x12345 -> 0x12345, but this is not expected
def pad16(hexNum):
    if ( hexNum[:2] != "0x" ) :
        exit(1)
    actualNum = hexNum[2:]
    length = len(actualNum)
    if ( length == 1 ) :
        return "000" + actualNum
    elif ( length == 2 ) :
        return "00" + actualNum
    elif ( length == 3 ) :
        return "0" + actualNum
    else :
        return hexNum

XMLREADER_START_ELEMENT_NODE_TYPE = 1

#reader = libxml2.newTextReaderFilename("../capture.xml")
#reader = libxml2.newTextReaderFilename("../newCap.xml")
#reader = libxml2.newTextReaderFilename("../mouse.xml")
#reader = libxml2.newTextReaderFilename("../nj.xml")

if len(sys.argv) != 2:
    print "Synopsis: " + sys.argv[0] + " <USBTraceLog.xml>"
    exit(1)

reader = libxml2.newTextReaderFilename(sys.argv[1])

# Old data for endpoints 0, 1 and 2
oldData00 = ""
oldData01 = ""
oldData02 = ""

# Parse through the XML...
while reader.Read():
    if reader.NodeType() == XMLREADER_START_ELEMENT_NODE_TYPE and \
            reader.Depth() == 1 and \
            reader.Name() == "Request":
        # This is a Request open tag...parse until the end tag
        node = reader.Expand()
        row = node.children
        
        # Initialise to empty the fields for this request
        rowMap = {
            "LogType": "",
            "Device Object": "",
            "EndpointAddress": "",
            "Index": "",
            "IRP": "",
            "Length": "",
            "PipeHandle": "",
            "Request": "",
            "RequestType": "",
            "RequestTypeReservedBits": "",
            "SetupPacket": "",
            "Status": "",
            "TransferBuffer": "",
            "TransferBufferLength": "",
            "TransferBufferMDL": "",
            "TransferFlags": "",
            "UrbLink": "",
            "USBD Status": "",
            "Value": "",
            "Data": ""
        }
        
        # Iterate through children. Expect only Data, LogType and Param
        while row is not None:
            if row.type == "element":
                if row.name == "Data":
                    # It's data - save it in the rowMap
                    rowMap["Data"] = row.content
                elif row.name == "LogType":
                    # It's a log type - save it
                    rowMap["LogType"] = row.content
                elif row.name == "Param":
                    # It's a name-value pair - save it
                    column = row.children
                    while column is not None:
                        if column.type == "element":
                            if column.name == "Name":
                                name = column.content
                            elif column.name == "Value":
                                value = column.content
                            else:
                                exit(1)
                        column = column.next
                    if name is None or value is None:
                        exit(1)
                    rowMap[name] = value
            row = row.next

        # OK so now we have all the data in the rowMap - process it
        if ( rowMap["LogType"] == "URB_FUNCTION_VENDOR_DEVICE" ) :
            # Remember the data - the data for writes precedes the control message so we need to remember it
            oldData00 = rowMap["Data"].strip()
        elif ( rowMap["LogType"] == "URB_FUNCTION_CONTROL_TRANSFER" ) :
            # For STATUS_SUCCESS rows, print an R0 or W0 row, with setup bytes and data, if any.
            # For STATUS_UNSUCCESSFUL rows, print an R0 or W0 row, with setup bytes and a "failed" note.
            # For STATUS_PENDING rows, print nothing
            if ( rowMap["Status"][:14] == "STATUS_SUCCESS" or rowMap["USBD Status"][:19] == "USBD_STATUS_SUCCESS") :
                #print "RequestType = \"" + rowMap["RequestType"] + "\"; Request = \"" + rowMap["Request"] + "\""
                setupBytes = rowMap["RequestType"][2:4] + " " + rowMap["Request"][2:] + " " + pad16(rowMap["Value"]) + " " + pad16(rowMap["Index"])
                if rowMap["RequestType"][:5] == "0xC0 ":
                    print "R0: " + setupBytes + " -> " + rowMap["Data"].strip()
                elif rowMap["RequestType"][:5] == "0x40 ":
                    print "W0: " + setupBytes + " -> " + oldData00
                elif rowMap["RequestType"][:5] != "0x80 ":
                    print "Unrecognised RequestType " + rowMap["RequestType"]
            elif rowMap["Status"][:19] == "STATUS_UNSUCCESSFUL":
                setupBytes = rowMap["RequestType"][2:4] + " " + rowMap["Request"][2:] + " " + pad16(rowMap["Value"]) + " " + pad16(rowMap["Index"])
                if ( rowMap["RequestType"][:5] == "0xC0 " ) :
                    print "R0: " + setupBytes + " failed"
                elif ( rowMap["RequestType"][:5] == "0x40 " ) :
                    print "W0: " + setupBytes + " failed"
                else :
                    print "Unrecognised RequestType " + rowMap["RequestType"]
        elif ( rowMap["LogType"] == "URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER" ) :
            # Print an R or a W row, with the appropriate endpoint, and the data.
            # TODO: Need a better way of deduplicating data written to bulk endpoints.
            #       Diffing is a bit crap because there might genuinely be two
            #       identical writes/reads.
            endpoint = rowMap["EndpointAddress"]
            data = rowMap["Data"].strip()
            if ( endpoint == "0x1" ) :
                if ( data != "" ) :
                    #print data + " <-> " + oldData01
                    if ( data != oldData01 ) :
                        print "W1: " + data
                    oldData01 = data
            elif ( endpoint == "0x81" ) :
                if ( data != "" ) :
                    print "R1: " + data
            elif ( endpoint == "0x2" ) :
                if ( data != "" ) :
                    if ( data != oldData02 ) :
                        print "W2: " + data
                    oldData02 = data
            elif ( endpoint == "0x82" ) :
                if ( data != "" ) :
                    print "R2: " + data
            elif ( endpoint == "0x86" ) :
                if ( data != "" ) :
                    print "R6: " + data
            else:
                print "Unrecognised endpoint " + endpoint
        elif ( rowMap["LogType"] != "URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE" ) :
            print "Unrecognised log entry " + rowMap["LogType"]
