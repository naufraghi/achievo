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
   * @copyright (c)2000-2004 Ivo Jansch
   * @copyright (c)2000-2004 Ibuildings.nl BV
   * @license http://www.achievo.org/atk/licensing ATK Open Source License
   *
   * @version $Revision: 5.22 $
   * $Id: class.atkadminhandler.inc,v 5.22 2006/05/25 01:43:59 guido Exp $
   */

  /**
   * Handler for the 'admin' action of a node. It displays a recordlist with
   * existing records, and links to view/edit/delete them (or custom actions
   * if present), and an embedded addform or a link to an addpage (depending
   * on the presence of the NF_ADD_LINK flag).
   *
   * @author Ivo Jansch <ivo@achievo.org>
   * @package atk
   * @subpackage handlers
   *
   */
  atkimport("atk.handlers.atkactionhandler");

  class atkAdminHandler extends atkActionHandler
  {
    /**
     * The action method
     */
    function action_admin()
    {
      $page = &$this->getPage();

      $res = array();
      if ($this->m_node->hasFlag(NF_NO_ADD)==false&&$this->m_node->allowed("add"))
      {
        if (!$this->m_node->hasFlag(NF_ADD_LINK)) // otherwise, in adminPage, an add link will be added.
        {
          $res[]=$this->invoke("addPage");
        }
      }
      $res[] = $this->invoke("adminPage");

      $page->addContent($this->m_node->renderActionPage("admin", $res));
    }

    /**
     * Draws the form for adding new records.
     *
     * The implementation delegates drawing of the form to the atkAddHandler.
     *
     * @return String A box containing the add page.
     */
    function addPage()
    {
      // Reuse the atkAddHandler for the addPage.
      $handler = atknew("atk.handlers.atkaddhandler");
      $handler->setNode($this->m_node);
      return $handler->invoke("addPage");
    }

    /**
     * Admin page displays records and the actions that can be performed on
     * them (edit, delete)
     *
     * @param array $actions The list of actions displayed next to each
     *                       record. Nodes can implement a
     *                       recordActions($record, &$actions, &$mraactions)
     *                       method to add/remove record-specific actions.
     * @return String A box containing the admin page (without the add form,
     *                which is added later.
     */
    function adminPage($actions="")
    {
      $node = &$this->m_node;
      $node->addStyle("style.css");

      $recordlistcache = $this->getRecordlistCache();
      $cached = "";
      if ($this->m_node->hasFlag(NF_CACHE_RECORDLIST) && $recordlistcache)
      {
        $cached = $recordlistcache->getCache();
      }

      if ($cached)
      {
        atkdebug("Using recordlist cache: {$recordlistcache->m_cacheid}");
        return $cached;
      }

      $ui = &$this->getUi();
      if (is_object($ui))
      {
        $params = array();

        // When there's a lot of data, records will be spread across multiple
        // pages.
        if (!isset($node->m_postvars['atklimit'])) $node->m_postvars['atklimit']=atkconfig("recordsperpage");
        if (!isset($node->m_postvars['atkstartat'])) $node->m_postvars['atkstartat']=0;

        $columnConfig = &$node->getColumnConfig();

        $atkfilter = atkArrayNvl($node->m_postvars, 'atkfilter', "");
        $recordset = $node->selectDb($atkfilter,
                                     $columnConfig->getOrderByStatement(),
                                     array("offset" => $node->m_postvars['atkstartat'], "limit" => $node->m_postvars['atklimit']),
                                     $node->m_listExcludes,
                                     "",
                                     "admin");

        //if the startat var is to big to get results back
        if(count($recordset) == 0 && $node->m_postvars['atkstartat'] != 0)
        {
          $node->m_postvars['atkstartat'] = 0;
          $recordset = $node->selectDb($atkfilter,
                                     $node->m_postvars['atkorderby'],
                                     array("offset" => $node->m_postvars['atkstartat'], "limit" => $node->m_postvars['atklimit']),
                                     $node->m_listExcludes,
                                     "",
                                     "admin");
        }


        // check for search-action (if search returns only one record!)
        if (isset($node->m_postvars["atksearch"]) && is_array($node->m_postvars['atksearch'])
            && count($node->m_postvars['atksearch']) > 0 && is_array($node->m_search_action) && count($recordset) == 1)
        {
          for ($i = 0, $_i = count($node->m_search_action); $i < $_i; $i++)
            if ($node->allowed($node->m_search_action[$i], $recordset[$i]))
            {
              $node->redirect(session_url($_SERVER["PHP_SELF"].'?atknodetype='.$node->atkNodeType().
                              '&atkaction='.$node->m_search_action[$i].
                              '&atkselector='.rawurlencode($node->primaryKey($recordset[0])),
                               SESSION_REPLACE));
              break;
            }
        }

        $params["header"] = $this->invoke("adminHeader");

        if ($this->getHeaderLinks()) $params["header"].= $this->getHeaderLinks();

        // create index and navigation bar
        $navigator = &atknew("atk.utils.atknavigator");
        if ($node->m_index != "")
        {
          $params["index"] = $navigator->buildIndex($node, (count($recordset)>0?$recordset[0][$node->m_index]:""));
        }

        $navigation = $navigator->getNavigationUrls($this->m_node, "admin");

        if ($navigation)
        {
          if (isset($params["index"])) $params["index"].="<br />";
          $params["navbar"] = $navigator->buildNavigation($this->m_node, "admin", $navigation);
        }

        if(!is_array($actions))
        {
          $actions = $node->defaultActions("admin");
        }
        $recordlist = &atknew("atk.recordlist.atkrecordlist");
        $flags = atkRecordList::convertFlags($node->getFlags());
        $params["list"] = $recordlist->render($node, $recordset, $actions, $flags, "", "", $navigation);

        $params["footer"] = $this->invoke("adminFooter");

        $output = $ui->renderList("admin", $params);

        $output = $ui->renderBox(array("title"=>$node->actionTitle('admin'),
                                    "content"=>$output));
        if ($this->m_node->hasFlag(NF_CACHE_RECORDLIST)) $recordlistcache->writeCache($output, $recordlist->m_actionloader);
      }
      else
      {
        atkerror("Failed to create ui object");
      }
      return $output;
    }

    /**
     * Function that is called when creating an adminPage.
     *
     * The default implementation returns an empty string, but developers can
     * override this function in their custom handlers or directly in the
     * node class.
     *
     * @return String A string that is displayed above the recordlist.
     */
    function adminHeader()
    {
      return "";
    }

   /**
     * Function that is called when creating an adminPage.
     *
     * The default implementation returns an empty string, but developers can
     * override this function in their custom handlers or directly in the
     * node class.
     *
     * @return String A string that is displayed below the recordlist.
     */
    function adminFooter()
    {
      return "";
    }

    function getImportLink()
    {
      $link = "";
      if ($this->m_node->allowed("add")&&!$this->m_node->hasFlag(NF_READONLY) && $this->m_node->hasFlag(NF_IMPORT))
      {
        $link.= href(dispatch_url($this->m_node->atkNodeType(), "import"), atktext("import", "atk", $this->m_node->m_type), SESSION_NESTED);
      }
      return $link;
    }

    function getExportLink()
    {
      $link = "";
      if ($this->m_node->allowed("view")&& $this->m_node->hasFlag(NF_EXPORT))
      {
        $link.= href(dispatch_url($this->m_node->atkNodeType(), "export"), atktext("export", "atk", $this->m_node->m_type), SESSION_NESTED);
      }
      return $link;
    }

    function getAddLink()
    {
      if ($this->m_node->hasFlag(NF_ADD_LINK) && !$this->m_node->hasFlag(NF_NO_ADD) && $this->m_node->allowed("add"))
      {
        $addurl = $_SERVER["PHP_SELF"].'?atknodetype='.$this->m_node->atknodetype().'&atkaction=add';
        $label = atktext("link_".$this->m_node->m_type."_add", $this->m_node->m_module, "", "", "", true);
        if (empty($label))
        {
          // generic text
          $label = atktext(getNodeType($this->m_node->m_type),$this->m_node->m_module,$this->m_node->m_type)." ".atktext("add","atk","");
        }
        return href($addurl,$label,SESSION_NESTED);
      }
      return "";
    }

    function getHeaderLinks()
    {
      $links = array();
      $addlink = $this->getAddLink();
      if ($addlink!="") $links[] = $addlink;
      $importlink = $this->getImportLink();
      if ($importlink!="") $links[] = $importlink;
      $exportlink = $this->getExportLink();
      if ($exportlink!="") $links[] = $exportlink;
      return implode(" | ", $links);
    }
  }

?>