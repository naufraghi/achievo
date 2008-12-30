<?php

  /**
   * This file is part of the Achievo ATK distribution.
   * Detailed copyright and licensing information can be found
   * in the doc/COPYRIGHT and doc/LICENSE files which should be 
   * included in the distribution.
   *
   * @package atk
   * @subpackage handlers
   *
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.5 $
   * $Id: class.atkeditcopyhandler.inc,v 5.5 2007/02/13 14:11:35 peter Exp $
   */
   
  /**
   * Handler for the 'editcopy' action of a node. It copies the selected
   * record, and then redirects to the edit action for the copied record.
   *
   * @author Peter C. Verhage <peter@ibuildings.nl>
   * @package atk
   * @subpackage handlers
   *
   */
  class atkEditCopyHandler extends atkActionHandler
  {
    /**
     * The action method. 
     */
    function action_editcopy()
    {
      atkdebug("atknode::action_editcopy()");
      $recordset = $this->m_node->selectDb($this->m_postvars['atkselector'],"","","","","copy");

      if(count($recordset) > 0)
      {
        // allowed to editcopy record?
        if (!$this->allowed($recordset[0]))
        {
          $this->renderAccessDeniedPage();
          return;
        }
        
        $db = &$this->m_node->getDb();
        if(!$this->m_node->copyDb($recordset[0]))
        {
          $db->rollback();
          $location = $this->m_node->feedbackUrl("editcopy", ACTION_FAILED, $recordset[0], $db->getErrorMsg());
          $this->m_node->redirect($location);
        }
        else
        {
          $db->commit();
          $this->clearCache();
          $location = session_url(dispatch_url($this->m_node->atknodetype(), "edit", array("atkselector" => $this->m_node->primaryKey($recordset[0]))), SESSION_REPLACE);
          $this->m_node->redirect($location);
        }
      }

      else
      {
        atkdebug("Geen records gevonden met selector: $selector");
        $this->m_node->redirect();
      }
    }
  }

?>