#!/usr/bin/env python3
"""Exit nonzero for stale/missing device heartbeats; suitable for cron monitoring."""
import argparse
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

def check(dbfile, expected, max_age_hours):
    if not dbfile.exists():
        return [f"missing database {dbfile}"]
    with sqlite3.connect(dbfile) as db:
        rows={d:(seen,ok) for d,seen,ok in db.execute(
            "SELECT device,last_seen,last_success FROM heartbeats")}
    now=datetime.now(timezone.utc)
    failures=[]
    for d in expected:
        if d not in rows:
            failures.append(f"{d}: no heartbeat");continue
        seen,success=rows[d]
        if not success:
            failures.append(f"{d}: never successfully backed up");continue
        age=(now-datetime.fromisoformat(success)).total_seconds()/3600
        if age>max_age_hours: failures.append(f"{d}: last success {age:.1f} hours ago")
    return failures

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--database",type=Path,required=True)
    ap.add_argument("--devices",nargs="+",required=True)
    ap.add_argument("--hours",type=float,default=48)
    args=ap.parse_args()
    problems=check(args.database,args.devices,args.hours)
    if problems: raise SystemExit("\n".join(problems))
    print("All devices healthy")
