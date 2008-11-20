<?php
/*
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id:$
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

/*
SELECT userid, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                SELECT person.userid, usercontract.uc_hours/5 as time_contract, project.name AS project_name,
                    phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
                    (project.name LIKE 'd20_%') AS d20
                FROM hours
                    JOIN person ON hours.userid = person.id
                    JOIN phase ON hours.phaseid = phase.id
                    JOIN activity ON hours.activityid = activity.id
                    JOIN project ON phase.projectid = project.id
                    JOIN usercontract ON usercontract.userid = person.id
                    WHERE activitydate > '2008-08-05'
                GROUP BY person.userid, usercontract.uc_hours, hours.activitydate, malattia, ferie, d20
            ) AS lavoro
        GROUP BY userid, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, activitydate, time_contract
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

function get_num_workdays($date_start, $date_end)
{
    $start_date = date_create($date_start);
    $stop_date = date_create($date_end);
    $curr_date = date_create($date_start);
    $workdays = 0;
    while ($curr_date <= $stop_date)
    {
        if (0 < $curr_date->format("w") & $curr_date->format("w") < 6)
            $workdays += 1;
        $curr_date->modify("+1 day");
    }
    return $workdays;
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
        $last_month_start = mktime(0, 0, 0, date("m")-1, 1, date("Y"));
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
                SELECT person.userid, usercontract.uc_hours/5 as time_contract, project.name AS project_name,
                    phase.name AS phase_name, hours.activitydate, sum(hours.time) AS time_spent,
                    (project.name = '__dipendenti' AND phase.name = 'malattia') AS malattia,
                    (project.name = '__dipendenti' AND phase.name = 'ferie') AS ferie,
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
                GROUP BY person.userid, usercontract.uc_hours, hours.activitydate, malattia, ferie, d20";

        $select = "
SELECT userid, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                $ore_registrate
            ) AS lavoro
        GROUP BY userid, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, activitydate, time_contract";

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

        $workdays = get_num_workdays($results["date_start"], $results["date_end"]);

        $content .= "Da ".$startdateatt->edit()." a ".$enddateatt->edit();
        $content .= "<h2>$title</h2>\n";
        $content .= "<p>".date_create()->format("Y-m-d")."</p>\n";

        // Tabella ore lavorate per progetto
        $content .= "<table border='1'>\n";
        $content .= "<tr><th>User</th><th>Giorno</th><th>Ore previste</th><th>Ore lavorate</th>
                         <th>Malattia</th><th>Ferie</th><th>Tickets</th>
                     </tr>\n";
        foreach ($results['rows'] as $key => $r)
        {
            $username = $r['userid'];
            $activitydate = $r['activitydate'];
            $contract_hours = str_number_format($r['time_contract'], 2);
            $time_worked = str_number_format($r['time_worked'], 2);
            $time_malattia = str_number_format($r['time_malattia'], 2);
            $time_ferie = str_number_format($r['time_ferie'], 2);
            $tickets = $r['tickets'];

            if (flipflop())
            {
                $bgcolor = "#44DD44";
            }
            else
            {
                $bgcolor = "#99DD99";
            }

            $content .= "<tr style='background: $bgcolor'>";
            $content .= "<td>$username</td>";
            $content .= "<td>$activitydate</td>";
            $content .= "<td align='right'>$contract_hours</td>";
            $content .= "<td align='right'>$time_worked</td>";
            $content .= "<td align='right'>$time_malattia</td>";
            $content .= "<td align='right'>$time_ferie</td>";
            $content .= "<td align='right'>$tickets</td>";
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
