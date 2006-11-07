<?php
  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be 
   * included in the distribution.
   *
   * This file is the skeleton menu loader, which you can copy to your
   * application dir and modify if necessary. By default, it checks
   * the menu settings and loads the proper menu.
   *
   * @package atk
   * @subpackage skel
   *
   * @author Ivo Jansch <ivo@achievo.org>
   * @author Peter C. Verhage <peter@ibuildings.nl>
   *
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 4.27 $
   * $Id: menu.php,v 4.27 2005/11/15 22:07:51 ivo Exp $
   */

  /**
   * @internal includes
   */
  $config_atkroot = "./";
  include_once("atk.inc");    

  atksession();
  atksecure();
  require "theme.inc";

  
  atkimport("atk.ui.atktheme");
  
  $output = &atkOutput::getInstance();
  $page = &atknew("atk.ui.atkpage");
  $theme = &atkTheme::getInstance();  
  $ui = &atknew("atk.ui.atkui");


  /* general menu stuff */
  /* load menu layout */
  atkimport("atk.menu.atkmenu");
  $menu = &atkMenu::getMenu();
  
  if (is_object($menu)) $output->output($menu->render());
  else atkerror("no menu object created!");;

  $output->outputFlush();

?>
