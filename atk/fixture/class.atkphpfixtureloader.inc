<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be
   * included in the distribution.
   *
   * @package atk
   * @subpackage fixture
   *
   * @copyright (c)2007 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 1.3 $
   * $Id: class.atkphpfixtureloader.inc,v 1.3 2007/08/30 08:08:04 ivo Exp $
   */

  /**
   * PHP fixture loader. Loads fixtures that are plain PHP
   * files. PHP files have access to a "global" variable named
   * $data that should be filled with the fixture data.
   *
   * @package atk
   * @subpackage fixture
   * @author Peter C. Verhage <peter@ibuildings.nl>
   */
  class atkPHPFixtureLoader extends atkAbstractFixtureLoader
  {
    /**
     * Loads and returns the fixture data from the given file.
     *
     * @param string $path fixture file path
     * @return array fixture data
     */
    function load($path)
    {
      $data = array();
      include_once($path);
      return $data;
    }
  }
?>