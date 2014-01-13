QHexEdit
========

This is a QHexEdit widget for Qt Framework 5, it is released under MIT License.

Features
-----
- Big File Support.
- Unlimited Undo/Redo.
- Fully Customizable.
- Easy to Use.

Known Bugs
-----
- Drawing Routines must be improved.

Usage
-----
The data is managed by QHexEdit through QHexEditData class.<br>
You can load a generic QIODevice using the method fromDevice() of QHexEditData class, also, helper methods are provided in order to load a QFile a In-Memory buffer in QHexEdit:<br>
```
/* Load data from In-Memory Buffer... */
QHexEditData* hexeditdata = QHexEditData::fromBuffer(bytearray);

/* ...or from a Generic I/O Device... */
QHexEditData* hexeditdata = QHexEditData::fromDevice(iodevice);

/* ...or from File */
QHexEditData* hexeditdata = QHexEditData::fromFile("data.bin");

QHexEdit* hexedit = new QHexEdit();
hexedit->setData(hexeditdata); /* Associate QHexEditData with this QHexEdit */
```