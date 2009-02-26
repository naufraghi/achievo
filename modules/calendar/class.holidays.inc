<?php
  /**
   * holidays class file
   *
   * @package achievo
   * @subpackage modules
   *
   * @author ivo <ivo@ibuildings.nl>
   * @author guido <guido@ibuildings.nl>
   *
   * @copyright (c) 2005 Ibuildings.nl BV
   * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2
   *
   * @version $Revision: 1.3 $
   * $Id: class.holidays.inc,v 1.3 2006/07/11 09:20:54 ivo Exp $
   */

  // Load the used attribute classes
  useattrib("atkdateattribute");

  /**
   * holidays node class
   *
   * @author ivo <ivo@ibuildings.nl>
   * @package achievo
   * @subpackage modules
   */
  class holidays extends atkNode
  {

    /**
     * holidays class constructor
     */
    function holidays()
    {
      $this->atkNode("holidays");

      $this->add(new atkDateAttribute("holiday_date", "","", 0, 0, AF_PRIMARY));
      $this->add(new atkAttribute("description", AF_OBLIGATORY));

      $this->setTable("holidays");
      $this->setOrder("holiday_date");
    }

  }
?>