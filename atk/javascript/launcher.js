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
   * @version $Revision: 5.1 $
   * $Id: launcher.js,v 5.1 2004/05/31 22:11:11 ivo Exp $
   */

function atkLaunchApp()
{
  if (window.screen)
  {
    var hori=screen.availWidth;
    var verti=screen.availHeight;
    window.open('app.php','fullscreen', 'width='+hori+',height='+verti+',fullscreen=1, scrollbars=0,left='+(0)+',top='+(0));
  }
}
