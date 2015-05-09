#ifndef SPARSERANGEMAP_H
#define SPARSERANGEMAP_H

#include <QtGlobal>
#include <QList>

template<class T> class SparseRangeMap
{
    private:
        struct Range
        {
            Range(): _start(-1), _end(-1) { }
            Range(qint64 start, qint64 end, const T& item) : _start(start), _end(end), _item(item) { }

            qint64 _start;
            qint64 _end;
            T _item;
        };

    public:
        void addRange(qint64 start, qint64 end, const T& item)
        {
            int i = this->internalClearRange(start, end);
            this->_sortedranges.insert(i, Range(start, end, item));
        }

        void clearRange(qint64 start, qint64 end)
        {
            this->internalClearRange(start, end);
        }

        const T* valueAt(qint64 pos) const
        {
            int idx = this->lookupRange(pos);

            if(idx >= 0)
                return &this->_sortedranges[idx]._item;

            return NULL;
        }

        void clear(){ _sortedranges.clear(); }
        bool contains(qint64 pos) const { return this->lookupRange(pos) >= 0; }

    private:
        int lookupRange(qint64 pos) const
        {
            int upper = this->_sortedranges.size() - 1, lower = 0, n = (upper - lower) + 1,  middle = n / 2;

            while(n > 0)
            {
                if(pos < this->_sortedranges[middle]._start)
                    upper = middle-1;
                else if(pos > this->_sortedranges[middle]._end)
                    lower = middle+1;
                else
                    return middle;

                n = (upper - lower) + 1;
                middle = n/2 + lower;
            }

            return -1;
        }

        int internalClearRange(qint64 start, qint64 end)
        {
            int insertidx = 0;
            bool findend = false;

            for(insertidx = 0; insertidx < this->_sortedranges.length(); insertidx++)
            {
                Range& r = this->_sortedranges[insertidx];

                if(end < r._start)
                    break;

                if(start <= r._start)
                {
                    if(end < r._end)
                        r._start = end+1; // Shrink start of existing and add new
                    else // end >= _sortedRanges[i]._end
                        findend = true; // We will replace existing and possibly others

                    break;
                }

                if(start <= r._end)
                {
                    if(end < r._end)
                    {
                        Range endPart(r);
                        endPart._start = end+1; // Split existing and insert new in between
                        r._end = start-1;

                        this->_sortedranges.insert(insertidx + 1, endPart);
                    }
                    else
                    {
                        r._end = start-1; // Shrink end of existing and search for end
                        findend = true;
                    }

                    insertidx++;
                    break;
                }
            }

            if(findend)
            {
                for(int i = insertidx; i < this->_sortedranges.length(); i++)
                {
                    if(end < this->_sortedranges[i]._start)
                        break;

                    if(end <= this->_sortedranges[i]._end)
                    {
                        // Shrink start of existing
                        this->_sortedranges[i]._start = end+  1;

                        if(this->_sortedranges[i]._start > this->_sortedranges[i]._end) // This is an invalid Range
                            this->_sortedranges.removeAt(i);

                        break;
                    }

                    this->_sortedranges.removeAt(i);
                    i--;
                }
            }

            return insertidx;
        }

    private:
        QList<Range> _sortedranges;
};

#endif // SPARSERANGEMAP_H
