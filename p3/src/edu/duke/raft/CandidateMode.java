package edu.duke.raft;

import java.util.Arrays;
import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {

	Timer electionCountTimer;
	int electionTimeout;
	final int electionTimerID = 2;

	public void go () {
		synchronized (mLock) {
			log("switched to candidate mode.");
			electionStart(); // This will invoke the requestVoteRPC of all other servers
		}
	}
	
	public void electionStart() {
		mConfig.setCurrentTerm(mConfig.getCurrentTerm() + 1, mID); // Increment term to indicate start of new election
		int term = mConfig.getCurrentTerm(); 
		
		if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
			electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}

		electionCountTimer = scheduleTimer(electionTimeout, electionTimerID);
		
		log("starting new election.");
		
		RaftResponses.clearVotes(term);
		RaftResponses.setTerm(term);
		// TODO: vote for self
		for (int id = 1; id <= mConfig.getNumServers(); id++) {
			if (id != mID) {
				this.remoteRequestVote(id, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
			}
		}
	}

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
			if (candidateTerm > mConfig.getCurrentTerm()) { // become a follower.
				electionCountTimer.cancel();
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				RaftServerImpl.setMode(new FollowerMode()); 
				return mConfig.getCurrentTerm();
			} else { // more stuff?
				return mConfig.getCurrentTerm();
			}
		}
	}


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
			if (leaderTerm >= mConfig.getCurrentTerm()) { // candidate is stale
				electionCountTimer.cancel();
				mConfig.setCurrentTerm(leaderTerm, leaderID);
				RaftServerImpl.setMode(new FollowerMode()); // cancel election, become a follower. 
				return mConfig.getCurrentTerm(); // we didn't append any entries, this is lower than leaders so shouldn't cause issues.
			} else {
				return mConfig.getCurrentTerm(); // leader is stale
			}
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				log("handling timeout");
				electionCountTimer.cancel();
				
				log("responses: "+Arrays.toString(RaftResponses.getVotes(mConfig.getCurrentTerm())));
		
				int[] responseVotes = RaftResponses.getVotes(mConfig.getCurrentTerm());
				int voteCount = 1; // voted for self.
				for (int id = 1; id < responseVotes.length; id++) {
					if (responseVotes[id] == 0) {
						voteCount++;
					} if (responseVotes[id] > mConfig.getCurrentTerm()) {
						RaftServerImpl.setMode(new FollowerMode());
					}
				}
				
				if (voteCount > mConfig.getNumServers()/2.0) { // 1. Won election (become leader)
					log("won election, new leader");
					RaftServerImpl.setMode(new LeaderMode());
				} else { // 3. Still waiting on vote (reset timeout) // new election?
					electionStart();
				}
			}
		}
	}

	public void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (candidate): " + message);
	}
}
