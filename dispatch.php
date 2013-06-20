<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be
   * included in the distribution.
   *
   * This file is the skeleton dispatcher file, which you can copy
   * to your application dir and modify if necessary. By default, it
   * checks the $atknodetype and $atkaction postvars and creates the
   * node and dispatches the action.
   *
   * @package atk
   * @subpackage skel
   *
   * @author Ivo Jansch <ivo@achievo.org>
   *
   * @copyright (c)2000-2004 Ivo Jansch
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 4.14 $
   * $Id: dispatch.php,v 4.14 2006/04/17 20:50:16 sandy Exp $
   */

  /**
   * @internal Setup the system
   */
  $config_atkroot = "./";
  include_once("atk.inc");

  atksession();

  $session = &atkSessionManager::getSession();

  if($ATK_VARS["atknodetype"]=="" || $session["login"]!=1)
  {
    // no nodetype passed, or session expired

    $page = &atknew("atk.ui.atkpage");
    atkimport("atk.ui.atkui");
    $ui = &atkUI::getInstance();
    $theme = &atkTheme::getInstance();
    $output = &atkOutput::getInstance();

    $page->register_style($theme->stylePath("style.css"));

    $destination = "index.php?atklogout=true";
    if(isset($ATK_VARS["atknodetype"]) && isset($ATK_VARS["atkaction"]))
    {
      $destination .= "&atknodetype=".$ATK_VARS["atknodetype"]."&atkaction=".$ATK_VARS["atkaction"];
      if (isset($ATK_VARS["atkselector"])) $destination .="&atkselector=".$ATK_VARS["atkselector"];
    }

    $title = atktext("title_session_expired");
    $contenttpl = '<br>%s<br><br><input type="button" onclick="top.location=\'%s\'" value="%s"><br><br>';
    $content = sprintf($contenttpl, atktext("explain_session_expired"), $destination, atktext("relogin"));
    $box = $ui->renderBox(array("title" => $title, "content" => $content));

    $page->addContent($box);

    $output->output($page->render(atktext("title_session_expired"), true));
  }
  else
  {
    atksecure();
    require "theme.inc";


    $lockType = atkconfig("lock_type");
    if (!empty($lockType)) atklock();

    // Create node
    $obj = &atkGetNode($ATK_VARS["atknodetype"]);

    if (is_object($obj))
    {
      $obj->dispatch($ATK_VARS);
    }
    else
    {
      atkdebug("No object created!!?!");
    }
  }
  $output = &atkOutput::getInstance();
  $output->outputFlush();
?>
