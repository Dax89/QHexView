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

/* Apply Comment */
hexedit->commentRange(0, 12, "I'm a comment!");

/* Remove a comment */
hexedit->uncommentRange(0, 5);

/* Clear all comments */
hexedit->clearComments();

/* Getting data */
QHexEditReader reader(hexeditdata);
quint32 a = reader.readUInt32(23); /* Read a 32 bit unsigned int from offset 23 using platform's byte order */
qint16 b = reader.readInt16(46, QSysInfo::BigEndian); /* Read a 16 bit unsigned short from offset 46 and convert it in Big Endian */
QByteArray data = reader.read(24, 78); /* Read 78 bytes starting from offset 24 */

/* Writing data */
QHexEditWriter writer(hexeditdata);
writer.insert(4, QString("Hello QHexEdit").toUTF8()); /* Insert an UTF-8 string from offset 4 */
writer.remove(6, 10);/* Delete bytes from offset 6 to offset 10 */
writer.replace(30, 12, QString("New Data").toUTF8()); /* Replace 12 bytes from offset 30 with the UTF-8 string "New Data" */

```