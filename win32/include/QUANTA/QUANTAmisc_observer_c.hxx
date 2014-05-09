/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

//  Based almost entirely on the work of Russel Winder

#ifndef _QUANTAmisc_observer_c
#define _QUANTAmisc_observer_c

#include <QUANTA/QUANTAconfig.h>


#ifdef QUANTA_STD_NAMESPACE
#include <list>
using namespace std;
#else /* QUANTA_STD_NAMESPACE */
#include <list.h>
#endif /* QUANTA_STD_NAMESPACE */


class QUANTAmisc_subject_c ;

/**
   Observer Design Pattern's Observer class.

   What is it for?  The subject-observer design pattern is an object
   oriented alternative to callback functions.  We can have multiple
   observers "watch" a single subject.  An observer begins to watch a
   subject when the subject makes an "attach(&observer)" call.  Everytime
   the subject calls its notify() method, all observers that are watching
   it will react by calling their update() method.

   We do not instantiate the QUANTAmisc_subject_c and the
   QUANTAmisc_observer_c.  Instead, the API user creates classes that
   inherit the properties of these two superclasses.  The class that
   inherits the observer class properties should implement the update()
   method.  The update method is a pure virtual function.  Not
   implementing it would result in a compiler error.

   @author: cavern@@evl

   @version: 12/1/99

   @sa QUANTAmisc_subject_c
*/
class QUANTAmisc_observer_c
{
public:

  virtual ~QUANTAmisc_observer_c() {} ;

  /** Triggered by an observered subject's notify() method.

      You can think of this as the object-oriented equivalent of a
      callback function.  This is a pure virtual function that should
      be implemented by classes that inherit from the observer.

      @param subj This is a CAVERnmisc_subject_c pointer to the
      subject that woke us up.  We can then access its public member
      variables and even execute its public methods.
  */
  virtual void update(QUANTAmisc_subject_c* subj) = 0 ;

protected:
  QUANTAmisc_observer_c() {}

};

/**
   Observer Design Pattern's Subject class.

   What is it for?  The subject-observer design pattern is an object
   oriented alternative to callback functions.  We can have multiple
   observers "watch" a single subject.  An observer begins to watch a
   subject when the subject makes an "attach(&observer)" call.  Everytime
   the subject calls its notify() method, all observers that are watching
   it will react by calling their update() method.

   We do not instantiate the QUANTAmisc_subject_c and the
   QUANTAmisc_observer_c.  Instead, the API user creates classes that
   inherit the properties of these two superclasses.  The class that
   inherits the observer class properties should implement the update()
   method.  The update method is a pure virtual function.  Not
   implementing it would result in a compiler error.

   @author: cavern@@evl

   @version: 12/1/99

   @sa QUANTAmisc_observer_c
*/
class QUANTAmisc_subject_c
{
public:
  virtual ~QUANTAmisc_subject_c() {};

  /**
     Subject calls attach(observer) to register the observer.  The
     observer is added to this Subject's observer list. After this,
     everytime the subject calls its notify() method, the registered
     observer reacts accordingly by calling its update() method.

     @param observer A pointer to the observer we want to register
  */
  virtual void attach(QUANTAmisc_observer_c *observer) ;

  /**
     Subject calls detach(observer) to DEregister the observer. It
     removes the observer from the subject's observer list.

     @param observer A pointer to the observer we want to unregister.
  */
  virtual void detach(QUANTAmisc_observer_c *observer) ;

  /**
     The subject's notify() method triggers its observers that an update
     has occurred. Each observer will call its update() method.
  */
  virtual void notify();

private:
  list<QUANTAmisc_observer_c *> m_ListOfObservers;
};


inline void QUANTAmisc_subject_c::attach(QUANTAmisc_observer_c *observer)
{
  m_ListOfObservers.push_back(observer);
}

inline void QUANTAmisc_subject_c::detach(QUANTAmisc_observer_c *observer)
{
  m_ListOfObservers.remove(observer);
}

inline void QUANTAmisc_subject_c::notify()
{
  // Iterate through this subject's list of obeserver and trigger an
  // update for each.
  list<QUANTAmisc_observer_c *>::iterator i ;
  for ( i = m_ListOfObservers.begin();
        i != m_ListOfObservers.end();
        ++i) {
    (*i)->update(this);
  }
}

#endif
