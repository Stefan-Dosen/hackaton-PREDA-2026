from flask import Flask, request, jsonify
from flask_cors import CORS
import json
import os

app = Flask(__name__)

# ✅ ADD THIS RIGHT HERE (immediately after app creation)
CORS(app)

latest_data = {
    "temperature": 0,
    "distance": 0,
    "mq2": 0,
    "mq3": 0,
    "rain": 0,
    "status": "OFFLINE"
}

DATA_FILE = "data.json"


@app.route("/api/data", methods=["POST"])
def receive_data():
    global latest_data

    data = request.json
    latest_data.update(data)

    with open(DATA_FILE, "w") as f:
        json.dump(latest_data, f)

    print("Data received:", latest_data)

    return jsonify({"result": "ok"})


@app.route("/api/data", methods=["GET"])
def get_data():
    return jsonify(latest_data)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)