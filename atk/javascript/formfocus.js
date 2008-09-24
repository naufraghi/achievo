  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be 
   * included in the distribution.
   *
   * @package atk
   * @subpackage javascript
   *
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.7 $
   * $Id: formfocus.js,v 5.7 2007/06/27 09:12:47 peter Exp $
   */
   
function placeFocus(inEditForm)
{
  if (typeof(inEditForm) == 'undefined')
    inEditForm = true;
    
  if (document.forms.length == 0) return;

  var fields = document.forms[0].elements;
  for (i = 0; i < fields.length; i++) 
  { 
    var field = fields[i];
    var type = field.type.toLowerCase();  
     
    if (type == "text" || type == "textarea" || type.toString().charAt(0) == "s") 
    {
      if (!inEditForm)
      {
        field.focus();
        break;
      }
      
      var found = false;
      
      var node = field.parentNode;
      while (node != null)
      {
        if (node.nodeName.toLowerCase() == 'tr')
        {
          
          found = node.id != null && node.id.substring(0, 3) == 'ar_' && node.style.display != 'none';
          if (found) 
          {
            try 
            {
              field.focus();
            }
            catch (err)
            {
              // ignore error
            }
          }
          break;
        }
        
        node = node.parentNode;
      }
      
      if (found) break;
    }
  }
}