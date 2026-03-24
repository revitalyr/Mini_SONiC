#!/usr/bin/env python3
"""
Performance Regression Checker for Mini SONiC

This script compares current benchmark results against baseline results
to detect performance regressions in the Mini SONiC system.
"""

import json
import argparse
import sys
from pathlib import Path
import numpy as np

class PerformanceRegressionChecker:
    def __init__(self, baseline_file, current_file, threshold=5.0):
        self.baseline_file = baseline_file
        self.current_file = current_file
        self.threshold = threshold
        self.baseline_data = self.load_results(baseline_file)
        self.current_data = self.load_results(current_file)
        self.regressions = []
        self.improvements = []
        
    def load_results(self, file_path):
        """Load benchmark results from JSON file"""
        try:
            with open(file_path, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"Error: Results file {file_path} not found")
            sys.exit(1)
        except json.JSONDecodeError:
            print(f"Error: Invalid JSON in {file_path}")
            sys.exit(1)
    
    def extract_benchmark_data(self, data):
        """Extract relevant benchmark data from JSON results"""
        benchmarks = {}
        
        for benchmark in data.get('benchmarks', []):
            name = benchmark.get('name', '')
            real_time = benchmark.get('real_time', 0)
            cpu_time = benchmark.get('cpu_time', 0)
            items_per_second = benchmark.get('items_per_second', 0)
            
            benchmarks[name] = {
                'real_time': real_time,
                'cpu_time': cpu_time,
                'items_per_second': items_per_second
            }
        
        return benchmarks
    
    def compare_performance(self):
        """Compare current performance against baseline"""
        baseline_benchmarks = self.extract_benchmark_data(self.baseline_data)
        current_benchmarks = self.extract_benchmark_data(self.current_data)
        
        for name, baseline_metrics in baseline_benchmarks.items():
            if name in current_benchmarks:
                current_metrics = current_benchmarks[name]
                
                # Compare throughput (items per second)
                baseline_throughput = baseline_metrics['items_per_second']
                current_throughput = current_metrics['items_per_second']
                
                if baseline_throughput > 0 and current_throughput > 0:
                    change = ((current_throughput - baseline_throughput) / baseline_throughput) * 100
                    
                    if change < -self.threshold:
                        self.regressions.append({
                            'benchmark': name,
                            'type': 'throughput',
                            'baseline': baseline_throughput,
                            'current': current_throughput,
                            'change_percent': change,
                            'severity': self.get_severity(change)
                        })
                    elif change > self.threshold:
                        self.improvements.append({
                            'benchmark': name,
                            'type': 'throughput',
                            'baseline': baseline_throughput,
                            'current': current_throughput,
                            'change_percent': change,
                            'severity': self.get_severity(change)
                        })
                
                # Compare latency (real time)
                baseline_latency = baseline_metrics['real_time']
                current_latency = current_metrics['real_time']
                
                if baseline_latency > 0 and current_latency > 0:
                    change = ((current_latency - baseline_latency) / baseline_latency) * 100
                    
                    if change > self.threshold:
                        self.regressions.append({
                            'benchmark': name,
                            'type': 'latency',
                            'baseline': baseline_latency,
                            'current': current_latency,
                            'change_percent': change,
                            'severity': self.get_severity(change)
                        })
                    elif change < -self.threshold:
                        self.improvements.append({
                            'benchmark': name,
                            'type': 'latency',
                            'baseline': baseline_latency,
                            'current': current_latency,
                            'change_percent': change,
                            'severity': self.get_severity(change)
                        })
    
    def get_severity(self, change_percent):
        """Determine severity level based on percentage change"""
        abs_change = abs(change_percent)
        if abs_change >= 20:
            return 'critical'
        elif abs_change >= 10:
            return 'high'
        elif abs_change >= 5:
            return 'medium'
        else:
            return 'low'
    
    def generate_report(self):
        """Generate performance regression report"""
        print("=" * 80)
        print("PERFORMANCE REGRESSION REPORT")
        print("=" * 80)
        print(f"Baseline: {self.baseline_file}")
        print(f"Current:  {self.current_file}")
        print(f"Threshold: {self.threshold}%")
        print()
        
        # Summary
        total_regressions = len(self.regressions)
        total_improvements = len(self.improvements)
        
        if total_regressions == 0:
            print("✅ NO PERFORMANCE REGRESSIONS DETECTED")
        else:
            print(f"❌ {total_regressions} PERFORMANCE REGRESSION(S) DETECTED")
        
        if total_improvements > 0:
            print(f"🚀 {total_improvements} PERFORMANCE IMPROVEMENT(S) DETECTED")
        
        print()
        
        # Detailed regressions
        if self.regressions:
            print("REGRESSIONS:")
            print("-" * 40)
            
            for regression in sorted(self.regressions, key=lambda x: abs(x['change_percent']), reverse=True):
                severity_emoji = {
                    'critical': '🔴',
                    'high': '🟠',
                    'medium': '🟡',
                    'low': '🟢'
                }.get(regression['severity'], '⚪')
                
                if regression['type'] == 'throughput':
                    baseline_val = regression['baseline']
                    current_val = regression['current']
                    unit = 'items/sec'
                else:  # latency
                    baseline_val = regression['baseline']
                    current_val = regression['current']
                    unit = 'ns'
                
                print(f"{severity_emoji} {regression['benchmark']}")
                print(f"   Type: {regression['type']}")
                print(f"   Baseline: {baseline_val:.2f} {unit}")
                print(f"   Current:  {current_val:.2f} {unit}")
                print(f"   Change:   {regression['change_percent']:+.2f}%")
                print(f"   Severity: {regression['severity']}")
                print()
        
        # Detailed improvements
        if self.improvements:
            print("IMPROVEMENTS:")
            print("-" * 40)
            
            for improvement in sorted(self.improvements, key=lambda x: abs(x['change_percent']), reverse=True):
                severity_emoji = {
                    'critical': '🚀',
                    'high': '⚡',
                    'medium': '📈',
                    'low': '📊'
                }.get(improvement['severity'], '📈')
                
                if improvement['type'] == 'throughput':
                    baseline_val = improvement['baseline']
                    current_val = improvement['current']
                    unit = 'items/sec'
                else:  # latency
                    baseline_val = improvement['baseline']
                    current_val = improvement['current']
                    unit = 'ns'
                
                print(f"{severity_emoji} {improvement['benchmark']}")
                print(f"   Type: {improvement['type']}")
                print(f"   Baseline: {baseline_val:.2f} {unit}")
                print(f"   Current:  {current_val:.2f} {unit}")
                print(f"   Change:   {improvement['change_percent']:+.2f}%")
                print(f"   Severity: {improvement['severity']}")
                print()
        
        # Summary statistics
        print("SUMMARY STATISTICS:")
        print("-" * 40)
        print(f"Total benchmarks compared: {len(self.baseline_data.get('benchmarks', []))}")
        print(f"Regressions detected: {total_regressions}")
        print(f"Improvements detected: {total_improvements}")
        
        if self.regressions:
            critical_regressions = sum(1 for r in self.regressions if r['severity'] == 'critical')
            high_regressions = sum(1 for r in self.regressions if r['severity'] == 'high')
            print(f"Critical regressions: {critical_regressions}")
            print(f"High regressions: {high_regressions}")
        
        print()
        
        # Return exit code based on regressions
        if total_regressions > 0:
            print("❌ PERFORMANCE REGRESSIONS FOUND - BUILD FAILED")
            return 1
        else:
            print("✅ NO REGRESSIONS - BUILD PASSED")
            return 0
    
    def save_report(self, output_file):
        """Save regression report to JSON file"""
        report = {
            'baseline_file': self.baseline_file,
            'current_file': self.current_file,
            'threshold': self.threshold,
            'regressions': self.regressions,
            'improvements': self.improvements,
            'summary': {
                'total_regressions': len(self.regressions),
                'total_improvements': len(self.improvements),
                'critical_regressions': sum(1 for r in self.regressions if r['severity'] == 'critical'),
                'high_regressions': sum(1 for r in self.regressions if r['severity'] == 'high')
            }
        }
        
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"Report saved to: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='Check for performance regressions in Mini SONiC')
    parser.add_argument('--baseline', '-b', required=True,
                        help='Baseline benchmark results JSON file')
    parser.add_argument('--current', '-c', required=True,
                        help='Current benchmark results JSON file')
    parser.add_argument('--threshold', '-t', type=float, default=5.0,
                        help='Regression threshold percentage (default: 5.0)')
    parser.add_argument('--output', '-o', default='regression_report.json',
                        help='Output report file (default: regression_report.json)')
    
    args = parser.parse_args()
    
    # Check if files exist
    if not Path(args.baseline).exists():
        print(f"Error: Baseline file {args.baseline} not found")
        sys.exit(1)
    
    if not Path(args.current).exists():
        print(f"Error: Current file {args.current} not found")
        sys.exit(1)
    
    # Run regression check
    checker = PerformanceRegressionChecker(args.baseline, args.current, args.threshold)
    checker.compare_performance()
    
    # Generate report
    exit_code = checker.generate_report()
    checker.save_report(args.output)
    
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
