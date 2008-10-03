<?php
/*
 * Copyright 2007 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id:$
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

useattrib("atkdateattribute");

function str_number_format($var, $digits)
{
    if (is_numeric($var))
        return number_format($var, $digits, ",", ".");
    else
        return $var;
}

//Supporto per la colorazione alternata delle righe
$_flipflop = true;
function flipflop()
{
    global $_flipflop;
    $_flipflop = !$_flipflop;
    return $_flipflop;
}

class timebalance extends atkNode
{
    function timesummary()
    {
        $this->atkNode("timesummary");
    }

    function get_timesummary()
    {
        global $g_user;
        $userid = $g_user["id"];

        $db = &atkGetDb();

        /* Accetta una data in ingresso oppure imposta l'intervallo all'ultimo mese appena concluso*/
        $last_month_start  = mktime(0, 0, 0, date("m")-1, 1, date("Y"));
        $last_month_end = mktime(0, 0, 0, date("m"), 0, date("Y"));
        $date_start = $this->m_postvars['date_start'];
        $date_end = $this->m_postvars['date_end'];
        if ($date_start == "")
            $date_start = date("Y-m-d", $last_month_start);
        else
            $date_start = sprintf("%04d",$date_start["year"])."-".sprintf("%02d",$date_start["month"])."-".sprintf("%02d",$date_start["day"]);
        if ($date_end == "")
            $date_end = date("Y-m-d", $last_month_end);
        else
            $date_end = sprintf("%04d",$date_end["year"])."-".sprintf("%02d",$date_end["month"])."-".sprintf("%02d",$date_end["day"]);

        if (!$this->allowed("any_user"))
            $single_user = "AND hours.userid ='$userid'";
        else
            $single_user = "";

        $ore_registrate = "
            SELECT person.userid, usercontract.uc_hours, project.name AS project_name,
                phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                (project.name = '__dipendenti' AND phase.name IN ('malattia', 'ferie')) AS nopay,
                (project.name LIKE 'd20_%') AS d20
            FROM hours
                JOIN person ON hours.userid = person.id
                JOIN phase ON hours.phaseid = phase.id
                JOIN activity ON hours.activityid = activity.id
                JOIN project ON phase.projectid = project.id
                JOIN usercontract ON usercontract.userid = person.id
            WHERE activitydate >= '$date_start'
                AND activitydate <= '$date_end'
                $single_user
            GROUP BY person.userid, usercontract.uc_hours, hours.activitydate, nopay, d20";

        $select = "
            SELECT userid, uc_hours, nopay, d20,
                sum(time_spent)/60 AS time_worked,
                sum(time_spent >= 5*60) AS tickets
            FROM (
                $ore_registrate
            ) AS ore_registrate
            GROUP BY LOWER(userid), uc_hours, nopay, d20";

        return array("date_start" => $date_start,
                     "date_end" => $date_end,
                     "rows" => $db->getRows($select));
    }

    function action_search()
    {
        atkdebug("timesummary Action search");

        $results = $this->get_timesummary();
        echo make_xml("timesummary", "$date_start<=D<=$date_end", $results['rows'], $bodykey="remark");
        exit;
    }

    function action_report()
    {
        $ui = &$this->getUi();
        $page = &$this->getPage();
        $this->addStyle("style.css");
        $content = '<form action="dispatch.php" method="get" name="entryform">';
        $content .= session_form();

        $results = $this->get_timesummary();
        $title = "TimeBalance: ".$results["date_start"]." - ".$results["date_end"];

        $startdateatt = new atkDateAttribute("date_start","F d Y","d F Y", 0, $results["date_start"], AF_OBLIGATORY);
        $enddateatt = new atkDateAttribute("date_end","F d Y","d F Y", 0, $results["date_end"], AF_OBLIGATORY);

        $content .= "Da ".$startdateatt->edit()." a ".$enddateatt->edit();
        $content .= "<h2>$title</h2>\n";

        // Tabella ore lavorate per progetto
        $content .= "<table border='1'>\n";
        $content .= "<tr><th>User</th><th>Ore giornaliere</th><th>Ore lavorate</th><th>Tikets</th></tr>\n";
        $progetti = array();
        $persone = array();
        foreach ($results['rows'] as $key => $r)
        {
            if ($r['nopay'])
                $content .= "<tr style='background: #FF4444'>";
            elseif ($r['d20'])
                $content .= "<tr style='background: #DDDD66'>";
            elseif (flipflop())
                $content .= "<tr style='background: #44DD44'>";
            else
                $content .= "<tr style='background: #99DD99'>";
            $content .= "<td>".$r['userid']."</td>";
            $content .= "<td align='right'>".str_number_format($r['uc_hours'], 2)."</td>";
            $content .= "<td align='right'>".str_number_format($r['time_worked'], 2)."</td>";
            if ($r['nopay'])
                $content .= "<td align='right'>nopay</td>";
            elseif ($r['d20'])
                $content .= "<td align='right'>d20</td>";
            else
                $content .= "<td align='right'>".$r['tickets']."</td>";
            $content .= "</tr>\n";
        }
        $content .= "</table>";

        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= '<input type="hidden" name="atknodetype" value="'.$this->atkNodeType().'">';
        $content .= '<input type="hidden" name="atkaction" value="report">';

        $content .= '</form>';
        $page->addContent($ui->renderBox(array("title"=>$title,"content"=>$content)));
    }
}
?>
