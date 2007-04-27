<?php
  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be 
   * included in the distribution.
   * 
   * @package atk
   * @subpackage utils
   *
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision:
   * $Id: class.atkmail.inc,v 1.2 2006/01/30 11:03:35 boy Exp $
   */

  /**
   * Utility for sending e-mails.
   *
   * @author Peter C. Verhage <peter@achievo.org> 
   *
   * @package atk
   * @subpackage utils
   */
  class atkMail
  {
    /**
     * Wrapper for the PHP mail function which accepts the exact same parameters 
     * as the normal mail function does but adds the ability to disable sending 
     * e-mails using a configuration variable.
     *
     * @return bool mail succesfully sent? 
     * @static
     */
    function mail()
    {
      if (atkconfig("mail_enabled", true))
      {
        $args = func_get_args();
        return call_user_func_array("mail", $args);
      }  
      else return true;
    }
  }
?>