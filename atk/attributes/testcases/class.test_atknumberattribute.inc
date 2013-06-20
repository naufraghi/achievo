<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be
   * included in the distribution.
   *
   * @package atk
   * @subpackage attribute
   * @access private
   *
   * @copyright (c)2007 Ibuildings
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * $Id: class.test_atknumberattribute.inc,v 1.1 2007/09/10 21:06:19 yury Exp $
   * @author Yury Golovnya  <ygolovnya@achievo.org>
   * @version $Revision: 1.1 $
   */


  /**
   * @internal includes
   */
  atkimport("atk.attributes.atknumberattribute");

  /**
   * Testcases for class atkNumberAttribute
   *
   * @author Yury Golovnya <ygolovnya@achievo.org>
   * @package atk
   * @subpackage attribute
   * @access private
   */
  class test_atknumberattribute extends atkAttributeTestCase
  {
    /**
     * Setup the testcase.
     */
    function setup()
    {
    }

    /**
     * Test method.
     */
    function test_attribute()
    {           
      $this->apiTest(new atknumberattribute(("name")));
    }              

    function test_getSearchCondition()
    {
      $db = &atkGetDb();
      $query = &$db->createQuery();
      
      $attribute = &new atkNumberAttribute("name");
      $value = 1;
      
      $searchString = $attribute->getSearchCondition($query,"table",$value,"substring");
      $etalonString = "UPPER(table.name) LIKE UPPER('%1%')";
      $this->assertEqual($searchString, $etalonString,"Search number 1 - %s");

      $value = 0;
      
      $searchString = $attribute->getSearchCondition($query,"table",$value,"substring");
      $etalonString = "UPPER(table.name) LIKE UPPER('%0%')";
      $this->assertEqual($searchString, $etalonString,"Search number 0 - %s");

      $value = "0";
      
      $searchString = $attribute->getSearchCondition($query,"table",$value,"substring");
      $etalonString = "UPPER(table.name) LIKE UPPER('%0%')";
      $this->assertEqual($searchString, $etalonString,"Search string '0' - %s");

      $value = "1";
      
      $searchString = $attribute->getSearchCondition($query,"table",$value,"substring");
      $etalonString = "UPPER(table.name) LIKE UPPER('%1%')";
      $this->assertEqual($searchString, $etalonString,"Search string '1' - %s");
    }      

  }
?>