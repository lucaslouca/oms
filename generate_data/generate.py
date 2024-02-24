from confluent_kafka import Producer
import socket
import json

KAFKA_CONF = {'bootstrap.servers': 'localhost:9092',
              'client.id': socket.gethostname()}

TOPIC = 'graph_data'


def graph():
    edges = []

    edges.append({
        "from": "a",
        "to": "b",
        "label": "A&B"
    })
    edges.append({
        "from": "a",
        "to": "d",
        "label": "A&D"
    })
    edges.append({
        "from": "b",
        "to": "c",
        "label": "B&C"
    })
    edges.append({
        "from": "d",
        "to": "c",
        "label": "D&C"
    })
    edges.append({
        "from": "d",
        "to": "g",
        "label": "D&G"
    })
    edges.append({
        "from": "g",
        "to": "h",
        "label": "G&H"
    })
    edges.append({
        "from": "g",
        "to": "i",
        "label": "G&I"
    })
    edges.append({
        "from": "h",
        "to": "i",
        "label": "H&I"
    })
    edges.append({
        "from": "h",
        "to": "k",
        "label": "H&K"
    })
    edges.append({
        "from": "c",
        "to": "i",
        "label": "C&I"
    })
    edges.append({
        "from": "c",
        "to": "e",
        "label": "C&E"
    })
    edges.append({
        "from": "c",
        "to": "f",
        "label": "C&F"
    })
    edges.append({
        "from": "e",
        "to": "f",
        "label": "E&F"
    })
    edges.append({
        "from": "f",
        "to": "j",
        "label": "F&J"
    })
    return edges


def main():
    producer = Producer(KAFKA_CONF)
    for i in range(1):
        for e in graph():
            producer.produce(TOPIC, key="a", value=json.dumps(e))
        producer.flush()


if __name__ == "__main__":
    main()
