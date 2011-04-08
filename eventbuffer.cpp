#include "eventbuffer.h"
#include "threadbuffer.h"

EventBuffer::EventBuffer (size_t size)
: Buffer_ (new ThreadBuffer<Event*> (size, 1, -1))
{
}

bool EventBuffer::empty () const {
    return Buffer_->available () == 0;
}

bool EventBuffer::full () const {
    return Buffer_->free () == 0;
}

size_t EventBuffer::size () const {
    return Buffer_->getSize ();
}

size_t EventBuffer::level () const {
    return Buffer_->available ();
}

void EventBuffer::setSize (size_t newsz) {
    ThreadBuffer<Event*>* newbuf = new ThreadBuffer<Event*> (newsz, 1, -1);
    ThreadBuffer<Event*>* oldbuf = Buffer_;
    Buffer_ = newbuf;
    delete oldbuf;
}

Event* EventBuffer::createEvent () {
    return new Event (this);
}

bool EventBuffer::queue (Event *ev) {
    return Buffer_->write(&ev, 1) == 1;
}

Event* EventBuffer::dequeue () {
    std::vector<Event*> rd (1, NULL);
    if (Buffer_->read(rd, 1) < 1)
        return NULL;

    return rd.front ();
}

EventSlot *EventBuffer::registerSlot (const AbstractModule *owner, QString name, PluginConnector::DataType type) {
    EventSlot *slot = new EventSlot (owner, name, type);
    if (Slots_.find (owner) == Slots_.end ()) // owning module not yet in registry
        Slots_.insert (owner, new SlotSet ());
    Slots_.value (owner)->insert (slot);
    return slot;
}

EventSlot *EventBuffer::getEventSlot (const AbstractModule *owner, QString name) const {
    SlotMap::const_iterator i;
    if ((i = Slots_.find (owner)) != Slots_.end ()) {
        SlotSet* s = i.value ();
        for (SlotSet::const_iterator j = s->begin (); j != s->end (); ++j) {
            if ((*j)->getName () == name)
                return *j;
        }
    }
    return NULL;
}

const QSet<EventSlot*>* EventBuffer::getEventSlots (const AbstractModule *owner) const {
    SlotMap::const_iterator i = Slots_.find (owner);
    return i != Slots_.end () ? i.value () : NULL;
}

void EventBuffer::destroyEventSlot (EventSlot *slot) {
    SlotMap::iterator i = Slots_.find (slot->getOwner ());
    if (i != Slots_.end ()) {
        SlotSet* s = i.value ();
        s->remove (slot);
        if (s->empty ()) { // remove the module entry from the registry
            Slots_.erase (i);
            delete s;
        }
    }
}

EventBuffer::~EventBuffer () {
    for (SlotMap::iterator i = Slots_.begin (); i != Slots_.end ();) {
        SlotSet *s = i.value ();
        i = Slots_.erase (i);

        for (SlotSet::iterator j = s->begin (); j != s->end ();) {
            EventSlot* slot = *j;
            j = s->erase (j);
            delete slot;
        }
        delete s;
    }

    delete Buffer_;
}

Event::Event (EventBuffer *buffer)
: EvBuf_ (buffer)
{
}

Event::~Event ()
{
}

void Event::put (const EventSlot *slot, QVariant data) {
    Data_.insert (slot, data);
}

QVariant Event::get(const EventSlot *slot) const {
    DataMap::const_iterator i = Data_.find (slot);
    if (i != Data_.end ())
        return i.value ();
    return QVariant ();
}

QSet<const EventSlot *> Event::getOccupiedSlots () const {
    QSet<const EventSlot *> ret;
    ret.reserve (Data_.size ());

    for (DataMap::const_iterator i = Data_.begin (); i != Data_.end (); ++i)
        if (!i.value ().isNull ())
            ret.insert (i.key ());
    return ret;
}

EventBuffer *Event::getBuffer () const {
    return EvBuf_;
}
