package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

/*
* Notes:
* Respond to RPCs from candidates and leaders
* If election timeout elapses without receiving AppendEntries
* RPC from current leader or granting vote to candidate:
* convert to candidate
*/

/* 5.2
* A server remains in follower state as long as it receives valid
* RPCs from a leader or candidate. Leaders send periodic
* heart beats (AppendEntries RPCs that carry no log entries)
* to all followers in order to maintain their authority. If a
* follower receives no communication over a period of time
* called the election timeout, then it assumes there is no viable
* leader and begins an election to choose a new leader.
*/

public class FollowerMode extends RaftMode {
	private Timer electionTimer;
	private final int electionTimerID = 1;

	private void setupElectionTimer() {
		int electionTimeout = mConfig.getTimeoutOverride();
		if (electionTimeout == -1) {
			electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}
		electionTimer = scheduleTimer(electionTimeout, electionTimerID);
	}

	public void go () {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm();
			System.out.println ("S" + 
					mID + 
					"." + 
					term + 
					": switched to follower mode.");

			setupElectionTimer();
		}
	}

	/* Reply false if term < currentTerm (§5.1)
	 * If votedFor is null or candidateId, and candidate’s log is at least as up-to-date as receiver’s log, grant vote (§5.2, §5.4)
	 * 
	 * Raft determines which of two logs is more up-to-date
	 * by comparing the index and term of the last entries in the
	 * logs. If the logs have last entries with different terms, then
	 * the log with the later term is more up-to-date. If the logs
	 * end with the same term, then whichever log is longer is
	 * more up-to-date. 
	 */
	
	// @param candidate’s term
	// @param candidate requesting vote
	// @param index of candidate’s last log entry
	// @param term of candidate’s last log entry
	// @return 0, if server votes for candidate; otherwise, server's
	// current term
	public int requestVote (int candidateTerm,
			int candidateID,
			int lastLogIndex,
			int lastLogTerm) {
		synchronized (mLock) {
			//log("vote request from server: " + candidateID + "." + candidateTerm);
			
			boolean candidateOlderTerm = candidateTerm < mConfig.getCurrentTerm();
			boolean candidateNewerTerm = candidateTerm > mConfig.getCurrentTerm();
			boolean candidateLogUpToDate = lastLogTerm >= mLog.getLastTerm();
			boolean haventVoted = (mConfig.getVotedFor() == 0 || mConfig.getVotedFor() == candidateID);
			
			if (candidateOlderTerm) {
				return mConfig.getCurrentTerm();
			} else if (candidateNewerTerm) {
				if (candidateLogUpToDate) {
					electionTimer.cancel();
					mConfig.setCurrentTerm(candidateTerm, candidateID);
					setupElectionTimer();
					return 0;
				}
				else {
					electionTimer.cancel();
					mConfig.setCurrentTerm(candidateTerm, 0);
					setupElectionTimer();
					return mConfig.getCurrentTerm();
				}
			} else {//Candidate Same Term
				if (haventVoted) {
	                if (candidateLogUpToDate) {
	                    electionTimer.cancel();
	                    setupElectionTimer();
	                    return 0;
	                }
	                else {
	                    electionTimer.cancel();
	                    mConfig.setCurrentTerm(candidateTerm, mID);
	                    RaftServerImpl.setMode(new CandidateMode());
	                    return mConfig.getCurrentTerm();
	                }
	            }
	            else {
	                return mConfig.getCurrentTerm();
	            }
			}
		}
	}

	/*
	 * Receiver implementation:
	 * 1. Reply false if term < currentTerm (§5.1)
 	 * 2. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm (§5.3)
     * 3. If an existing entry conflicts with a new one (same index but different terms), delete the existing entry and all that follow it (§5.3)
	 * 4. Append any new entries not already in the log
	 * 5. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry) !!!!
	 */
	
	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID,
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {
			if (leaderTerm < mConfig.getCurrentTerm()) {
				return mConfig.getCurrentTerm();
			}
			
			if (leaderTerm > mConfig.getCurrentTerm()) {
				mConfig.setCurrentTerm(leaderTerm, 0);
			}
			
			if (mLog.getEntry(prevLogIndex) == null || mLog.getEntry(prevLogIndex).term != prevLogTerm) {
				electionTimer.cancel();
				setupElectionTimer();
				return -2;
			} else {
				electionTimer.cancel();
				setupElectionTimer();
				
				mLog.insert(entries, prevLogIndex, prevLogTerm);
				
				if (leaderCommit > mCommitIndex) {
					mCommitIndex = Math.min(leaderCommit, mLog.getLastIndex());
				}
				
				if (entries.length == 0) {
					return -1;
				} else {
					return 0;
				}
			}
		}
	}  

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				//log("time is up! transition to candidate");
				electionTimer.cancel();
				RaftServerImpl.setMode(new CandidateMode());
			}
		}
	}

//	private void log(String message) {
//		int currentTerm = mConfig.getCurrentTerm();
//		System.out.println("S" + mID + "." + currentTerm + " (Follower Mode): " + message);
//	}
}

