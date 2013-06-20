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
   * @version $Revision: 5.2 $
   * $Id: class.atkmanytoonerelation.js,v 5.2 2004/10/09 16:47:18 sandy Exp $
   */

  function mto_parse(link, value)
  {
     var value_array = value.split('=');
     if(value_array[1]=='' || typeof value_array[1] == "undefined" ) return -1;
     var atkselector = value.replace("='", "_1253D_12527").replace("'", "_12527");
     return link.replace('REPLACEME', atkselector);
  }
