QHexEdit
========

This is an HexEdit widget for Qt Framework 5, it is released under MIT License.

Features
-----
- Big File Support.
- Unlimited Undo/Redo.
- Fully Customizable.
- Easy to Use.

Usage
-----
The data is managed by QHexEdit through QHexEditData class.<br>
You can load a generic QIODevice using the method fromDevice() of QHexEditData class, also, helper methods are provided in order to load a QFile a In-Memory buffer in QHexEdit:<br>
```
/* Load data from In-Memory Buffer (QHexEditData takes Ownership) ... */
QHexEditData* hexeditdata = QHexEditData::fromMemory(bytearray);

/* ...or from a Generic I/O Device... */
QHexEditData* hexeditdata = QHexEditData::fromDevice(iodevice);

/* ...or from File */
QHexEditData* hexeditdata = QHexEditData::fromFile("data.bin");

QHexEdit* hexedit = new QHexEdit();
hexedit->setData(hexeditdata); /* Associate QHexEditData with this QHexEdit */

/* Change Background Color */
hexedit->highlightBackground(0, 10, QColor(Qt::Red)); /* Highlight Background from 0 to 10 (10 included) */

/* Change Foreground Color */
hexedit->highlightForeground(0, 15, QColor(Qt::darkBLue)); /* Highlight from 0 to 15 (15 included) */

/* Clear Highlighting */
hexedit->clearHighlight();
```