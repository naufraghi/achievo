<?php
/*
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All rights reserved.
 *
 * $Id:$
 * Author: Matteo Bertini <naufraghi@develer.com>
 */

/*
SELECT userid, firstname, lastname, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, firstname, lastname, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                SELECT person.userid, person.firstname, person.lastname,
                    usercontract.uc_hours/5 as time_contract, project.name AS project_name,
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
                GROUP BY person.firstname, person.lastname, person.userid, usercontract.uc_hours,
                         hours.activitydate, malattia, ferie, d20
            ) AS lavoro
        GROUP BY userid, firstname, lastname, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, firstname, lastname, activitydate, time_contract
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
function flipflop($flip=true, $flop=false)
{
    global $_flipflop;
    $_flipflop = !$_flipflop;
    if ($_flipflop)
        return $flip;
    else
        return $flop;
}

function get_days_array($date_start, $date_end)
{
    $start_date = date_create($date_start);
    $stop_date = date_create($date_end);
    $curr_date = date_create($date_start);
    $days = array();
    while ($curr_date <= $stop_date)
    {
        array_push($days, $curr_date);
        $curr_date->modify("+1 day");
    }
    return $days;
}

function get_num_workdays($date_start, $date_end)
{
    $workdays = 0;
    foreach (get_days_array($date_start, $date_end) as $curr_date)
    {
        if (0 < $curr_date->format("w") & $curr_date->format("w") < 6)
            $workdays += 1;
    }
    return $workdays;
}

function f($float)
{
    return str_number_format($float, 2);
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
                SELECT person.userid, person.firstname, person.lastname,
                    usercontract.uc_hours/5 as time_contract, project.name AS project_name,
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
                GROUP BY person.firstname, person.lastname, person.userid, usercontract.uc_hours,
                         hours.activitydate, malattia, ferie, d20
        ";

        $select = "
SELECT userid, firstname, lastname, activitydate, time_contract, SUM(time_worked) as time_worked,
    SUM(time_malattia) as time_malattia, SUM(time_ferie) as time_ferie, SUM(time_d20) as time_d20,
    SUM(tickets) as tickets
    FROM (
        SELECT userid, firstname, lastname, activitydate, time_contract,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent)/60 AS time_worked,
            malattia * SUM(time_spent)/60 AS time_malattia,
            ferie * SUM(time_spent)/60 AS time_ferie,
            d20 * SUM(time_spent)/60 AS time_d20,
            (NOT (malattia OR ferie OR d20)) * SUM(time_spent > 5*60) AS tickets
            FROM (
                $ore_registrate
            ) AS lavoro
        GROUP BY userid, firstname, lastname, activitydate, time_contract, malattia, ferie, d20
    ) AS parts
GROUP BY userid, firstname, lastname, activitydate, time_contract;
";
        return array("date_start" => $date_start,
                     "date_end" => $date_end,
                     "rows" => $db->getRows($select));
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

        // Prepara dati in struttura 3d
        $time_balance = array();
        foreach ($results['rows'] as $key => $r)
        {
            if (!array_key_exists($r['userid'], $time_balance))
            {
                $time_balance[$r['userid']] = array('fullname' => $r['firstname']." ".$r['lastname'],
                                                    'activitydate' => array(),
                                                    'tot_contract' => $workdays*$r['time_contract'],
                                                    'tot_worked' => 0.0,
                                                    'tot_malattia' => 0.0,
                                                    'tot_ferie' => 0.0,
                                                    'tot_tickets' => 0,
                                                   );
            }
            array_push($time_balance[$r['userid']]['activitydate'],
                       array('date' => $r['activitydate'],
                             'time_contract' => $r['time_contract'],
                             'time_worked' => $r['time_worked'],
                             'time_malattia' => $r['time_malattia'],
                             'time_ferie' => $r['time_ferie'],
                             'tickets' => $r['tickets']));
            $time_balance[$r['userid']]['tot_worked'] += $r['time_worked'];
            $time_balance[$r['userid']]['tot_malattia'] += $r['time_malattia'];
            $time_balance[$r['userid']]['tot_ferie'] += $r['time_malattia'];
            $time_balance[$r['userid']]['tot_tickets'] += $r['tickets'];
        }
        // Tabelle banca ore
        foreach ($time_balance as $userid => $data)
        {
            $content .= "<table border='1'>\n";
            $content .= "<tr><th>User</th><th>Nome</th><th>Giorno</th><th>Ore previste</th>
                             <th>Ore lavorate</th><th>Malattia</th><th>Ferie</th><th>Tickets</th>
                         </tr>\n";
            foreach ($data['activitydate'] as $activity)
            {
                $bgcolor = flipflop("#44FF44", "#99FF99");

                $content .= "<tr style='background: $bgcolor'>";
                $content .= "<td>$userid</td>";
                $content .= "<td>".$data['fullname']."</td>";
                $content .= "<td>".$activity['date']."</td>";
                $content .= "<td align='right'>".f($activity['time_contract'])."</td>";
                $content .= "<td align='right'>".f($activity['time_worked'])."</td>";
                $content .= "<td align='right'>".f($activity['time_malattia'])."</td>";
                $content .= "<td align='right'>".f($activity['time_ferie'])."</td>";
                $content .= "<td align='right'>".$activity['tickets']."</td>";
                $content .= "</tr>\n";
            }
            $content .= "<tr><th>User</th><th>Nome</th><th>Totale</th>";
            $content .= "<th align='right'>".f($data['tot_contract'])."</th>";
            $content .= "<th align='right'>".f($data['tot_worked'])."</th>";
            $content .= "<th align='right'>".f($data['tot_malattia'])."</th>";
            $content .= "<th align='right'>".f($data['tot_ferie'])."</th>";
            $content .= "<th align='right'>".$data['tot_tickets']."</th>";
            $content .= "</tr>\n";
            $content .= "</table>\n";
        }

        $content .= '<input type="submit" value="'.atktext("refresh").'">';
        $content .= '<input type="hidden" name="atknodetype" value="'.$this->atkNodeType().'">';
        $content .= '<input type="hidden" name="atkaction" value="report">';

        $content .= '</form>';
        $page->addContent($ui->renderBox(array("title"=>$title,"content"=>$content)));
    }
}
?>
