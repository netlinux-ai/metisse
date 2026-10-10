#
# Contains a generic GTK application class that encapsulates some duplicate
# code frome each of the simple applications, as well as utility methods for the
# various accessiblity functions. 
#
# IMPORTANT: This module should be the first one imported in any ui facades
# accessibility application. Specifically, it should be imported before the
# pygtk and gtk modules, as the OS environment variable set below needs to be
# set before those modules are imported, as that enables GTK accessibility.
# Alternatively, you could, of course, set the variable directly.


import os
os.environ['GTK_MODULES'] = "gail:atk-bridge" # Make program visible to AT-SPI

import pygtk
pygtk.require('2.0')
import gtk
import bonobo

class AccessibleApp:
  '''
  Parent class encapsulating some of the duplicate code
  in simple gtk apps. It doesn't *really* have anything to do with
  accessibility.
  '''
  def destroy(self, widget, data=None):
    gtk.main_quit()
    
  def __init__(self):
    self.win = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.win.connect("destroy", self.destroy)
    self.win.set_border_width(10)
  
  def main(self):
    gtk.main()
    
class AccessorApp(AccessibleApp):
  '''
  Parent class for applications that have to access other applications
  '''
  def __init__(self):
    AccessibleApp.__init__(self)
    self.accessible_registry = bonobo.get_object("OAFIID:Accessibility_Registry:1.0", "Accessibility/Registry")
    
# Utility methods for accessing components. If there gets to be too many, should
# go in a separate module. Alternatively, some could be methods in the
# AccessorApp class
def find_comp_in_component(comp, comp_name, comp_role):
  '''
  given a parent component, comp, and the name and role of a component, recursively
  search the parent's widget hierarchy to locate the component in question
  '''
  for i in range(comp.childCount):
    child = comp.getChildAtIndex(i)
    if child.getRoleName() == comp_role:
      if child.name == comp_name:
        return child
    else:
      child_in_subtree = find_comp_in_component(child, comp_name, comp_role)
      if child_in_subtree:
        return child_in_subtree
  return None
      
    
def find_comp(desktop, application_name, comp_name, comp_role):
  '''
  given an accessible desktop, name of application, and component name and role,
  find the named application on the desktop and then find the named component
  in the application's widget hierarchy.
  '''
  for i in range(desktop.childCount):
    app = desktop.getChildAtIndex(i)
    if app.name == application_name:
      return find_comp_in_component(app, comp_name, comp_role)
      
def find_comp_with_role(comp, comp_role):
  '''
  Given a parent component, comp, and the role of a child component, recursively
  search for the first child of that component that has that role.
  Uses breadth-first search -- was originally created for menu bars, and it
  seems conceivable that multiple windows in an app might have menu bars, but
  the one nearest the top is the one you want.
  '''
  non_role_list = []
  for i in range(comp.childCount):
    child = comp.getChildAtIndex(i)
    if child.getRoleName() == comp_role:
      return child
    else:
      non_role_list.append(child)
      
  for child in non_role_list:
    search_child = find_comp_with_role(child, comp_role)
    if search_child:
      return search_child
  return None
