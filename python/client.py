#!/usr/bin/env python3

import argparse
import uuid
import json
import time

import pika

def parse_args():
    parser = argparse.ArgumentParser(description="Query market data ticks")
    parser.add_argument("--symbol", required=True, help="Full SYMBOL_NAME from minutely-bars.csv")
    parser.add_argument("--start", required=True, help="Beginning of time range")
    parser.add_argument("--end", required=True, help="End of time range")
    parser.add_argument("--timeout", type=float, default=3.0, help="Seconds to wait for a response")
    return parser.parse_args()

def main():
    args = parse_args()
    
    request_id = str(uuid.uuid4())
    payload = {
        "request_id": request_id,
        "symbol": args.symbol,
        "start_time": args.start,
        "end_time": args.end
    }
    
    credentials = pika.PlainCredentials("guest", "guest")
    parameters = pika.ConnectionParameters(
        host="localhost",
        port=5672,
        credentials=credentials
    )
    connection = pika.BlockingConnection(parameters)
    
    try:
        channel = connection.channel()
        callback_queue = channel.queue_declare(queue="", exclusive=True, auto_delete=True)
        reply_queue = callback_queue.method.queue # ID of per-session response queue
        
        response_body = None
        
        def on_response(channel, method, properties, body):
            nonlocal response_body # Use var declared outside on_response scope
            if properties.correlation_id != request_id:
                return
            
            response_body = json.loads(body)
            channel.basic_ack(method.delivery_tag)
        
        channel.basic_consume(queue=reply_queue, on_message_callback=on_response, auto_ack=False)
        channel.basic_publish(
            exchange="",
            routing_key="market_data_requests",
            body=json.dumps(payload),
            properties=pika.BasicProperties(
                content_type="application/json",
                correlation_id=request_id,
                reply_to=reply_queue
            )
        )
        
        timeout = time.monotonic() + args.timeout
        while response_body is None and time.monotonic() < timeout:
            connection.process_data_events(time_limit=1)
        
        if response_body is None:
            print(f"\nTimed out after {args.timeout} seconds awaiting response for\n\t{payload}\n")
            return 1
        
        print(json.dumps(response_body, indent=2))
        if response_body.get("status") == "error":
            return 1
        
        return 0
        
    finally:
        connection.close()

if __name__ == "__main__":
    raise SystemExit(main())
