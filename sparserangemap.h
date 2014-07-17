#ifndef SPARSERANGEMAP_H
#define SPARSERANGEMAP_H

#include <QtGlobal>
#include <QVector>

template <class T>
class SparseRangeMap
{
public:
    void addRange(qint64 start, qint64 end, const T& item);
    void clearRange(qint64 start, qint64 end);
    void clear() { _sortedRanges.clear(); }
    bool contains(qint64 pos) const { return lookupRange(pos) >= 0; }
    const T* valueAt(qint64 pos) const
    {
        int idx = lookupRange(pos);
        if(idx >= 0)
        {
            return &_sortedRanges[idx]._item;
        }
        else
        {
            return NULL;
        }
    }

private:
    int lookupRange(qint64 pos) const;
    int internalClearRange(qint64 start, qint64 end);

private:
    struct Range
    {
        Range()
            : _start(-1), _end(-1)
        {
        }

        Range(qint64 start, qint64 end, const T& item)
            : _start(start), _end(end), _item(item)
        {}

        qint64  _start;
        qint64  _end;
        T       _item;
    };

    QVector<Range> _sortedRanges;
};

template <class T>
void SparseRangeMap<T>::addRange(qint64 start, qint64 end, const T& item)
{
    int i = internalClearRange(start, end);
    _sortedRanges.insert(i, Range(start, end, item));
}

template <class T>
void SparseRangeMap<T>::clearRange(qint64 start, qint64 end)
{
    internalClearRange(start, end);
}

template <class T>
int SparseRangeMap<T>::lookupRange(qint64 pos) const
{
    int upper = _sortedRanges.size()-1;
    int lower = 0;
    int n = (upper - lower) + 1;
    int middle = n/2;

    while(n > 0)
    {
        if(pos < _sortedRanges[middle]._start)
        {
            upper = middle-1;

        }
        else if(pos > _sortedRanges[middle]._end)
        {
            lower = middle+1;
        }
        else
        {
            return middle;
        }

        n = (upper - lower) + 1;
        middle = n/2 + lower;
    }

    return -1;
}

template <class T>
int SparseRangeMap<T>::internalClearRange(qint64 start, qint64 end)
{
    int insertAt = 0;
    bool findEnd = false;

    for(insertAt=0; insertAt<_sortedRanges.size(); insertAt++)
    {
        Range& r = _sortedRanges[insertAt];
        if(end < r._start)
        {
            break;
        }
        else if(start <= r._start)
        {
            if(end < r._end)
            {
                // Shrink start of existing and add new
                r._start = end+1;
            }
            else // end >= _sortedRanges[i]._end
            {
                // We will replace existing and possibly others
                findEnd = true;
            }
            break;
        }
        else if(start <= r._end)
        {
            if(end < r._end)
            {
                // Split existing and insert new in between
                Range endPart(r);
                endPart._start = end+1;

                r._end = start-1;
                _sortedRanges.insert(insertAt+1, endPart);
            }
            else
            {
                // Shrink end of existing and search for end
                r._end = start-1;

                findEnd = true;
            }
            insertAt++;
            break;
        }
    }

    if(findEnd)
    {
        for(int i=insertAt; i<_sortedRanges.size(); i++)
        {
            if(end < _sortedRanges[i]._start)
            {
                break;
            }
            else if(end <= _sortedRanges[i]._end)
            {
                // Shrink start of existing
                _sortedRanges[i]._start = end+1;
                break;
            }
            else
            {
                _sortedRanges.remove(i);
                i--;
            }
        }
    }

    return insertAt;
}

#endif // SPARSERANGEMAP_H
