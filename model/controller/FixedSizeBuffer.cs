using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DreamCoreV2_model_controller
{
    public class FixedSizeBuffer<T>
    {
        private int _size = 0;
        private T?[] buffer;
        private int rptr;
        private int wptr;

        public FixedSizeBuffer(int size)
        {
            _size = size;
            buffer = new T?[size];
        }

        public bool IsEmpty()
        {
            return rptr == wptr;
        }

        public bool IsFull()
        {
            return (wptr + _size - rptr) % _size == (_size - 1);
        }

        public int Count()
        {
            return (wptr + _size - rptr) % _size;
        }

        public void Pop()
        {
            if(!IsEmpty())
            {
                rptr++;

                if(rptr >= _size)
                {
                    rptr = 0;
                }
            }
        }

        public void Push(T item)
        {
            if(IsFull())
            {
                Pop();
            }

            buffer[wptr++] = item;

            if(wptr >= _size)
            {
                wptr = 0;
            }
        }

        public void Clear()
        {
            rptr = 0;
            wptr = 0;
        }

        public T? Get(int index)
        {
            return buffer[(rptr + index) % _size];
        }
    }
}
