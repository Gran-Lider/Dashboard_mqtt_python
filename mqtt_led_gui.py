import sys

import paho.mqtt.client as mqtt
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QApplication,
    QGridLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QWidget,
)


TOPIC_LED = "casa/led"


class MqttLedWindow(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.client = mqtt.Client(client_id="pyqt_led_controller")
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect

        self.setWindowTitle("Control LED ESP32 por MQTT")
        self.resize(420, 180)

        layout = QGridLayout(self)

        self.host_input = QLineEdit("192.168.18.11")
        self.port_input = QLineEdit("1883")
        self.status_label = QLabel("Desconectado")
        self.status_label.setAlignment(Qt.AlignCenter)

        self.connect_button = QPushButton("Conectar")
        self.connect_button.clicked.connect(self.connect_to_broker)

        self.on_button = QPushButton("Encender LED")
        self.on_button.clicked.connect(lambda: self.publish_command("ON"))
        self.on_button.setEnabled(False)

        self.off_button = QPushButton("Apagar LED")
        self.off_button.clicked.connect(lambda: self.publish_command("OFF"))
        self.off_button.setEnabled(False)

        layout.addWidget(QLabel("Broker MQTT:"), 0, 0)
        layout.addWidget(self.host_input, 0, 1, 1, 2)
        layout.addWidget(QLabel("Puerto:"), 1, 0)
        layout.addWidget(self.port_input, 1, 1)
        layout.addWidget(self.connect_button, 1, 2)
        layout.addWidget(self.status_label, 2, 0, 1, 3)
        layout.addWidget(self.on_button, 3, 0, 1, 3)
        layout.addWidget(self.off_button, 4, 0, 1, 3)

    def on_connect(self, client, userdata, flags, rc):
        connected = rc == 0
        self.status_label.setText("Conectado" if connected else f"Error de conexion: rc={rc}")
        self.on_button.setEnabled(connected)
        self.off_button.setEnabled(connected)

    def on_disconnect(self, client, userdata, rc):
        self.status_label.setText("Desconectado")
        self.on_button.setEnabled(False)
        self.off_button.setEnabled(False)

    def connect_to_broker(self) -> None:
        host = self.host_input.text().strip()
        try:
            port = int(self.port_input.text().strip())
        except ValueError:
            QMessageBox.warning(self, "Puerto invalido", "Ingresa un puerto numerico valido.")
            return

        try:
            self.client.connect(host, port, 60)
            self.client.loop_start()
            self.status_label.setText("Intentando conectar...")
        except Exception as exc:
            QMessageBox.critical(self, "Error MQTT", f"No se pudo conectar al broker:\n{exc}")

    def publish_command(self, command: str) -> None:
        if not self.client.is_connected():
            QMessageBox.warning(self, "Sin conexion", "Primero conectate al broker MQTT.")
            return

        result = self.client.publish(TOPIC_LED, command)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            self.status_label.setText(f"Comando enviado: {command}")
        else:
            QMessageBox.warning(self, "Error al publicar", f"No se pudo enviar {command}.")

    def closeEvent(self, event) -> None:
        if self.client.is_connected():
            self.client.disconnect()
        self.client.loop_stop()
        event.accept()


def main() -> int:
    app = QApplication(sys.argv)
    window = MqttLedWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
