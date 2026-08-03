Transport Adapters
==================

The framework provides dependency-free MQTT and CAN adapter headers. They convert trivially-copyable
C++ values to binary payloads and route inbound values through ``Dispatcher::setDatapoint``. This
preserves generated access control and range validation without selecting a networking stack or CAN
driver for the application.

Both adapters use the in-memory representation of the value. Peers must therefore agree on byte order,
floating-point representation, struct layout, and model versions. For interoperable protocols, define
an application-level payload encoding before sending values to another architecture.

Change Notifications
--------------------

Every datapoint accepts one optional allocation-free callback. It is invoked after a successful write,
including writes that enter through a group mapping or dispatcher.

.. code-block:: cpp

   void onTemperature(const Temperature& value, void* context) noexcept;

   temperature.setChangeCallback(onTemperature, applicationContext);
   std::ignore = temperature.set(Temperature{ .raw = 2310, .value = 23.1f });
   temperature.clearChangeCallback();

The caller owns callback lifetime, synchronization, and ISR/thread-safety decisions. The callback is
not invoked when a range check rejects the value.

MQTT
----

Include ``mqttAdapter.h`` and implement ``DataLayer::Mqtt::Client`` using the MQTT client library
chosen by the application. The framework does not connect, subscribe, or manage reconnects.

.. code-block:: cpp

   class ApplicationMqttClient final : public DataLayer::Mqtt::Client
   {
   public:
       bool publish(std::string_view topic, std::span<const std::byte> payload, bool retain) noexcept override;
   };

   ApplicationMqttClient client;
   DataLayer::Mqtt::Adapter mqtt{ client, Dispatcher };

   mqtt.publish("plant/temperature", temperature());
   mqtt.applySet<Temperature>(temperature.getId(), incomingPayload);

``publish`` returns ``DataLayer::Mqtt::Result::publishFailed`` when the client rejects the message.
``applySet`` rejects payloads with the wrong byte size and reports writes that fail dispatcher access,
type, or range validation as ``rejected``.

CAN and CAN FD
--------------

Include ``canAdapter.h`` and implement ``DataLayer::Can::Bus`` for the target CAN driver. A
``DataLayer::Can::Frame`` contains an identifier, byte length, and a payload with capacity for a CAN FD
frame.

.. code-block:: cpp

   class ApplicationCanBus final : public DataLayer::Can::Bus
   {
   public:
       bool send(const DataLayer::Can::Frame& frame) noexcept override;
   };

   ApplicationCanBus bus;
   DataLayer::Can::Adapter can{ bus, Dispatcher };       // standard CAN: 8-byte payload limit
   DataLayer::Can::Adapter<decltype(Dispatcher), 64> canFd{ bus, Dispatcher };

   can.publish(temperature.getId(), temperature());
   can.applySet<Temperature>(receivedFrame);

The frame identifier is passed directly to the dispatcher as the datapoint ID. Applications should
reserve and filter identifiers consistently with the model's group base IDs. ``publish`` returns
``payloadTooLarge`` when the typed value does not fit the selected capacity. Use the default adapter
for classic CAN and a capacity of ``64`` for CAN FD.
