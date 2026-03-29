#!/usr/bin/env python3

import io
from pathlib import Path

import psycopg2

def main():
    project_root = Path(__file__).resolve().parents[1]
    csv_path = project_root / "data" / "minutely-bars.csv"
    
    connection = psycopg2.connect(
        host="localhost",
        port="5432",
        dbname="app_db",
        user="postgres",
        password="postgres"
    )

    try:
        with connection:
            with connection.cursor() as cursor:
                cursor.execute("TRUNCATE market_quotes RESTART IDENTITY") # Reset table so it doesn't get populated twice
                with csv_path.open("r", newline="") as handle:
                    stringified_csv = io.StringIO("".join(line for line in handle if line.strip())) # Only include non-blank lines
                    cursor.copy_expert(
                        """
                        COPY market_quotes (
                            symbol_name,
                            time,
                            ask_price,
                            ask_size,
                            bid_price,
                            bid_size
                            )
                        FROM STDIN
                        WITH (FORMAT csv, HEADER true)
                        """,
                        stringified_csv
                    )
        print("Success")
    
    finally:
        connection.close()

if __name__ == "__main__":
    main()
