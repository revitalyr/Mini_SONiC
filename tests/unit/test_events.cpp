/**
 * @file test_events.cpp
 * @brief Unit tests for MiniSonic.Events module
 */

#include <catch2/catch_all.hpp>

// Import the module
import MiniSonic.Events;

using namespace MiniSonic::Events;

TEST_CASE("Events PacketInfo Serialization", "[events][packet]") {
    SECTION("PacketInfo to JSON") {
        PacketInfo info{
            .id = 12345,
            .src_mac = "aa:bb:cc:dd:ee:ff",
            .dst_mac = "ff:ee:dd:cc:bb:aa",
            .src_ip = "192.168.1.10",
            .dst_ip = "192.168.1.20",
            .src_port = 12345,
            .dst_port = 80,
            .protocol = "TCP",
            .dscp = 0,
            .ttl = 64
        };
        
        auto json = info.toJson();
        
        REQUIRE(json["id"] == 12345);
        REQUIRE(json["src_mac"] == "aa:bb:cc:dd:ee:ff");
        REQUIRE(json["dst_ip"] == "192.168.1.20");
        REQUIRE(json["protocol"] == "TCP");
    }
}

TEST_CASE("Events PacketGenerated", "[events][packet]") {
    SECTION("PacketGenerated event creation and serialization") {
        PacketInfo info{
            .id = 1,
            .src_mac = "aa:bb:cc:dd:ee:01",
            .dst_mac = "ff:ff:ff:ff:ff:ff",
            .src_ip = "10.0.0.1",
            .dst_ip = "10.0.0.2",
            .src_port = 0,
            .dst_port = 0,
            .protocol = "ARP",
            .dscp = 0,
            .ttl = 64
        };
        
        PacketGenerated event(1234567890, info, "switch1");
        
        REQUIRE(event.type == "PacketGenerated");
        REQUIRE(event.timestamp_ns == 1234567890);
        REQUIRE(event.ingress_switch == "switch1");
        
        auto json = event.toJson();
        REQUIRE(json["type"] == "PacketGenerated");
        REQUIRE(json["ingress_switch"] == "switch1");
        REQUIRE(json["packet"]["id"] == 1);
    }
}

TEST_CASE("Events PacketEnteredSwitch", "[events][pipeline]") {
    SECTION("PacketEnteredSwitch event") {
        PacketEnteredSwitch event(1234567890, "switch1", 100, "port1");
        
        REQUIRE(event.type == "PacketEnteredSwitch");
        REQUIRE(event.packet_id == 100);
        REQUIRE(event.ingress_port == "port1");
        
        auto json = event.toJson();
        REQUIRE(json["type"] == "PacketEnteredSwitch");
        REQUIRE(json["packet_id"] == 100);
        REQUIRE(json["ingress_port"] == "port1");
    }
}

TEST_CASE("Events PacketForwardDecision", "[events][pipeline]") {
    SECTION("PacketForwardDecision event") {
        PacketForwardDecision event(
            1234567890, "switch1", 100, "port2", 
            "route1", "hash123", "10.0.0.1"
        );
        
        REQUIRE(event.type == "PacketForwardDecision");
        REQUIRE(event.packet_id == 100);
        REQUIRE(event.egress_port == "port2");
        REQUIRE(event.next_hop == "10.0.0.1");
        
        auto json = event.toJson();
        REQUIRE(json["type"] == "PacketForwardDecision");
        REQUIRE(json["egress_port"] == "port2");
        REQUIRE(json["next_hop"] == "10.0.0.1");
    }
}

TEST_CASE("Events PacketExitedSwitch", "[events][pipeline]") {
    SECTION("PacketExitedSwitch event") {
        PacketExitedSwitch event(1234567890, "switch1", 100, "port2");
        
        REQUIRE(event.type == "PacketExitedSwitch");
        REQUIRE(event.packet_id == 100);
        REQUIRE(event.egress_port == "port2");
        
        auto json = event.toJson();
        REQUIRE(json["type"] == "PacketExitedSwitch");
        REQUIRE(json["egress_port"] == "port2");
    }
}

TEST_CASE("Events PipelineStageEntered", "[events][pipeline]") {
    SECTION("PipelineStageEntered event") {
        PipelineStageEntered event(1234567890, "switch1", 100, "L2");
        
        REQUIRE(event.type == "PipelineStageEntered");
        REQUIRE(event.stage == "L2");
        
        auto json = event.toJson();
        REQUIRE(json["stage"] == "L2");
    }
}

TEST_CASE("Events PipelineStageExited", "[events][pipeline]") {
    SECTION("PipelineStageExited event") {
        PipelineStageExited event(1234567890, "switch1", 100, "L2", 1000);
        
        REQUIRE(event.type == "PipelineStageExited");
        REQUIRE(event.latency_ns == 1000);
        
        auto json = event.toJson();
        REQUIRE(json["latency_ns"] == 1000);
    }
}

TEST_CASE("Events PacketDropped", "[events][pipeline]") {
    SECTION("PacketDropped event") {
        PacketDropped event(1234567890, "switch1", 100, "No route");
        
        REQUIRE(event.type == "PacketDropped");
        REQUIRE(event.reason == "No route");
        
        auto json = event.toJson();
        REQUIRE(json["reason"] == "No route");
    }
}

TEST_CASE("Events PortStateChanged", "[events][switch]") {
    SECTION("PortStateChanged event") {
        PortStateChanged event(1234567890, "switch1", "port1", "up");
        
        REQUIRE(event.type == "PortStateChanged");
        REQUIRE(event.port == "port1");
        REQUIRE(event.state == "up");
        
        auto json = event.toJson();
        REQUIRE(json["port"] == "port1");
        REQUIRE(json["state"] == "up");
    }
}

TEST_CASE("Events EventBus SubscribeAndPublish", "[events][bus]") {
    SECTION("Subscribe and publish event") {
        EventBus bus;
        bool callback_called = false;
        nlohmann::json received_json;
        
        bus.subscribe("TestEvent", [&](const nlohmann::json& json) {
            callback_called = true;
            received_json = json;
        });
        
        nlohmann::json json_event = {{"type", "TestEvent"}, {"value", 42}};
        bus.publishJson(json_event);
        
        REQUIRE(callback_called);
        REQUIRE(received_json["type"] == "TestEvent");
        REQUIRE(received_json["value"] == 42);
    }
}

TEST_CASE("Events EventBus PublishJson", "[events][bus]") {
    SECTION("Publish JSON directly") {
        EventBus bus;
        bool callback_called = false;
        
        bus.subscribe("TestEvent", [&](const nlohmann::json& json) {
            callback_called = true;
            REQUIRE(json["type"] == "TestEvent");
        });
        
        nlohmann::json json_event = {{"type", "TestEvent"}, {"value", 42}};
        bus.publishJson(json_event);
        
        REQUIRE(callback_called);
    }
}

TEST_CASE("Events GlobalEventBus", "[events][bus]") {
    SECTION("Get global event bus instance") {
        auto& bus = getGlobalEventBus();
        bool callback_called = false;
        
        bus.subscribe("GlobalTest", [&](const nlohmann::json& json) {
            callback_called = true;
        });
        
        nlohmann::json json_event = {{"type", "GlobalTest"}};
        bus.publishJson(json_event);
        
        REQUIRE(callback_called);
    }
}
